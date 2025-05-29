using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using UnityEditor.Playables;
using System.Reflection;
using System;
using HybridCLR.Editor.Commands;
using HybridCLR.Editor;
using UnityEditorInternal;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using YooAsset.Editor;
using YooAsset;
using Newtonsoft.Json;
using System.Xml;
using HybridCLR.Editor.AOT;
using HybridCLR.Editor.Meta;
using HybridCLR.Editor.Settings;
using UnityEditor.Build.Pipeline.Utilities;

public class BuildHelper
{
    public static string BuildOutputPath=AssetBundleBuilderHelper.GetDefaultBuildOutputRoot();
    /// <summary>
    /// 工程目录路径，Assets上一层
    /// </summary>
    public static string ProjectPath =Directory.GetParent(Application.dataPath).FullName;
    
    public static string PackageExportPath = $"{ProjectPath}/BuildPacakage/";
    
    /// <summary>
    /// 版本文件名
    /// </summary>
    public static string VersionFileName = "/VERSION.txt";
    
    // Start is called before the first frame update
    public static string[] GetBuildScenes()
    {
        List<string> names = new List<string>();
        foreach (EditorBuildSettingsScene e in EditorBuildSettings.scenes)
        {
            if (e == null)
                continue;
            if (e.enabled)
                names.Add(e.path);
        }
        return names.ToArray();
    }



    
    [MenuItem("整合工具/打APK包")]
    public static void Debug_BuildAPK()
    {
        //先生成AOT文件，再进行打包，以确保所有引用库都被引用,废弃，因HybridCLR会修改构建管线，自动执行一次GenerateALL
        PrebuildCommand.GenerateAll();

        BuildAPK();
    }

    public static void BuildAPK()
    {
        BuildPlayerOptions buildPlayerOptions = new BuildPlayerOptions();
        buildPlayerOptions.scenes = GetBuildScenes();

        var versionString = File.ReadAllText(Application.streamingAssetsPath + VersionFileName);

        var buildPath = $"{PackageExportPath}{PlayerSettings.productName}_{versionString}_{DateTime.Now.ToString("yyyy_M_d_HH_mm_s")}";
        var buildTarget = EditorUserBuildSettings.activeBuildTarget;

        switch (buildTarget)
        {
            case BuildTarget.Android:
                buildPath = buildPath + ".apk";
                break;
        }
        buildPlayerOptions.locationPathName = buildPath;
        buildPlayerOptions.target = buildTarget;
        buildPlayerOptions.options = BuildOptions.None;
        //执行打包 场景名字，打包路径
        BuildPipeline.BuildPlayer(buildPlayerOptions);

        EditorUtility.ClearProgressBar();
    }
    
    /// <summary>
    /// 获取AOT之前,应先编译热更新代码
    /// 执行之前需要先编译热更新代码 CompileDllCommand.CompileDllActiveBuildTarget()
    /// </summary>
    public static void GetPatchedAOTAssemblyListToHybridCLRSettings()
    {
        var gs = SettingsUtil.HybridCLRSettings;
        List<string> hotUpdateDllNames = SettingsUtil.HotUpdateAssemblyNamesExcludePreserved;

        AssemblyReferenceDeepCollector collector = new AssemblyReferenceDeepCollector(MetaUtil.CreateHotUpdateAndAOTAssemblyResolver(EditorUserBuildSettings.activeBuildTarget, hotUpdateDllNames), hotUpdateDllNames);
        var analyzer = new Analyzer(new Analyzer.Options
        {
            MaxIterationCount = Math.Min(20, gs.maxGenericReferenceIteration),
            Collector = collector,
        });

        analyzer.Run();

        var types = analyzer.AotGenericTypes.ToList();
        var methods = analyzer.AotGenericMethods.ToList();

        List<dnlib.DotNet.ModuleDef> modules = new HashSet<dnlib.DotNet.ModuleDef>(
            types.Select(t => t.Type.Module).Concat(methods.Select(m => m.Method.Module))).ToList();
        modules.Sort((a, b) => a.Name.CompareTo(b.Name));

        List<string> patchtedAOTAssemblys = new List<string>();
        foreach (dnlib.DotNet.ModuleDef module in modules)
        {
            //替换掉程序集的拓展名,以方便后续拷贝AOTDll的时候可以和HotUpdateDll共用相同的拷贝逻辑
            var patchtedAOTAssemblysName = module.Name.Replace(".dll",string.Empty);
            Debug.Log($"需要补充元数据的AOT ========= {patchtedAOTAssemblysName}");
            patchtedAOTAssemblys.Add(patchtedAOTAssemblysName);
        }

        gs.patchAOTAssemblies = patchtedAOTAssemblys.ToArray();
    }
    [MenuItem("整合工具/获取需要补充元数据的Dll")]
    public static void Debug_GetPatchedAOTAssemblyList()
    {
        CompileDllCommand.CompileDllActiveBuildTarget();

        GetPatchedAOTAssemblyListToHybridCLRSettings();
    }

    public static List<string> CopyDllFileToByte(string[] originFileNames,string originDir,string targetDir)
    {
        List<string> bytesFiles = new List<string>();
        foreach (var originFileName in originFileNames)
        {
            var dllFilePath = Path.Combine(ProjectPath,originDir, $"{originFileName}.dll");
            if (!File.Exists(dllFilePath))
            {
                Debug.Log($"{dllFilePath}不存在");
                continue;
            }
            var targetFileName= $"{originFileName}.bytes";
            var dllRawFilePath = Path.Combine(targetDir, targetFileName);
            File.Copy(dllFilePath,dllRawFilePath,true);
            bytesFiles.Add(targetFileName);
        }

        return bytesFiles;
    }

    /// <summary>
    /// 将生成裁剪后的AOT dlls拷贝到AssetBundle打包路径下
    /// 依赖于   HybridCLR/Generate/Il2CppDef
    /// HybridCLR/Generate/LinkXmlH
    /// ybridCLR/Generate/AotDlls  三条指令生成数据
    /// </summary>
    /// <param name="rawFileCollectPath"></param>
    public static void CopyPatchedAOTDllToCollectPath(string rawFileCollectPath)
    {
        if (string.IsNullOrEmpty(rawFileCollectPath))
        {
            Debug.unityLogger.LogError("CopyPatchedAOTDllToCollectPath",$"{nameof(rawFileCollectPath)}===>Null");
            return;
        }
        
        var patchedAOTAssemblies = SettingsUtil.HybridCLRSettings.patchAOTAssemblies;
        
        var dllOutputPath = SettingsUtil.GetAssembliesPostIl2CppStripDir(EditorUserBuildSettings.activeBuildTarget);

        var dllRawFileAssetNames=CopyDllFileToByte(patchedAOTAssemblies,dllOutputPath,rawFileCollectPath);
  
        if (dllRawFileAssetNames != null && dllRawFileAssetNames.Count > 0)
        {
            var namesJson= JsonConvert.SerializeObject(dllRawFileAssetNames);
            File.WriteAllText($"{rawFileCollectPath}/AOTDLLs.txt", namesJson);
            AssetDatabase.Refresh();
            Debug.unityLogger.Log("CopyPatchedAOTDllToCollectPath Success!");   
        }
        else
        {
            Debug.unityLogger.LogError("CopyPatchedAOTDllToCollectPath",$"{nameof(dllRawFileAssetNames)}===>Null");
        }

    }
    
    [MenuItem("整合工具/生成AOT补充文件并复制进文件夹")]
    public static void Debug_GenerateAOTDllListFile()
    {
        //先生成AOT文件
        Il2CppDefGeneratorCommand.GenerateIl2CppDef();
        LinkGeneratorCommand.GenerateLinkXml();
        StripAOTDllCommand.GenerateStripedAOTDlls();

        var aotDllRawFileCollectPath = Path.Combine(Application.dataPath, "HotUpdateAssets","AOTDLL");
        
        Debug.unityLogger.Log(aotDllRawFileCollectPath);
        CopyPatchedAOTDllToCollectPath(aotDllRawFileCollectPath);
    }

    [MenuItem("整合工具/生成热更新Dll并复制进文件夹")]
    public static void Debug_GenerateHotUpdateDllListFile()
    {
        CompileDllCommand.CompileDllActiveBuildTarget();
        
        var hotUpdateDllRawFileCollectPath = Path.Combine(Application.dataPath, "HotUpdateAssets","HotUpdateDLL");
        
        Debug.unityLogger.Log(hotUpdateDllRawFileCollectPath);
        CopyHotUpdateDllToCollectPath(hotUpdateDllRawFileCollectPath);
    }
    
    /// <summary>
    /// 将生成裁剪后的HotUpdate dlls拷贝到AssetBundle打包路径下
    /// 依赖于   CompileDllCommand.CompileDllActiveBuildTarget()  生成数据
    /// </summary>
    /// <param name="rawFileCollectPath"></param>
    public static void CopyHotUpdateDllToCollectPath(string rawFileCollectPath)
    {
        if (string.IsNullOrEmpty(rawFileCollectPath))
        {
            Debug.unityLogger.LogError("CopyHotUpdateDllToCollectPath",$"{nameof(rawFileCollectPath)}===>Null");   
            return;
        }
        var hotUpdateAssemblies = SettingsUtil.HotUpdateAssemblyNamesExcludePreserved;

        var hotUpdateOutputPath =
            SettingsUtil.GetHotUpdateDllsOutputDirByTarget(EditorUserBuildSettings.activeBuildTarget);

        var dllRawFileAssetNames=CopyDllFileToByte(hotUpdateAssemblies.ToArray(),hotUpdateOutputPath,rawFileCollectPath);

        if (dllRawFileAssetNames != null && dllRawFileAssetNames.Count > 0)
        {
            var json = JsonConvert.SerializeObject(dllRawFileAssetNames);
            File.WriteAllText(Path.Combine(rawFileCollectPath,"HotUpdateDLLs.txt"), json);
            AssetDatabase.Refresh();
            Debug.unityLogger.Log("CopyHotUpdateDllToCollectPath  Success");   
        }
        else
        {
            Debug.unityLogger.LogError("CopyHotUpdateDllToCollectPath",$"{nameof(dllRawFileAssetNames)}===>Null");
        }
    }


    //[UnityEditor.UnityEditor.MenuItem("整合工具/删除本地沙盒文件夹")]
    public static void DeleteSandBoxDirectory()
    {
        var path = $"{ProjectPath}/SandBox";
        if (Directory.Exists(path))
        {
            Directory.Delete(path, true);
        }
        Debug.Log("沙盒文件夹删除成功");
    }
    [UnityEditor.MenuItem("整合工具/删除本地AB包数据并重新创建版本文件")]
    public static void DeleteAssetBundlesDataAndVersionFile()
    {
        DeleteSandBoxDirectory();
        CreateVersionFile();
    }

    [MenuItem("整合工具/创建版本文件")]
    public static void CreateVersionFile()
    {
        string version = "1.0.0";
        File.WriteAllText(Application.streamingAssetsPath + VersionFileName, version);
        Debug.Log("创建版本文件完成，当前版本为:" + version);
    }

    [MenuItem("整合工具/补全热更新预制体依赖")]
    public static void SupplementPrefabDependent()
    {
        EditorUtility.DisplayProgressBar("Progress", "Find Class...", 0);
        string[] dirs = { "Assets/HotUpdateAssets" };
        var asstIds = AssetDatabase.FindAssets("t:Prefab", dirs);
        var count = 0;
        Dictionary<string, List<string>> increasinglyAssemblyDic = new Dictionary<string, List<string>>();
        //遍历所有预制体
        for (int i = 0; i < asstIds.Length; i++)
        {
            string path = AssetDatabase.GUIDToAssetPath(asstIds[i]);
            var pfb = AssetDatabase.LoadAssetAtPath<GameObject>(path);

            var coms = pfb.GetComponentsInChildren<Component>();
            //遍历预制体所有组件
            foreach (var com in coms)
            {
                var type = com.GetType();
                var Fields = type.GetFields();
                string typeName = type.FullName;
                var assemblyName = type.Assembly.GetName();
                if (typeName.StartsWith("UnityEngine") || typeName.StartsWith("TMPro"))
                {
                    if (!increasinglyAssemblyDic.ContainsKey(assemblyName.Name))
                    {
                        increasinglyAssemblyDic.Add(assemblyName.Name, new List<string>());
                    }
                    if (!increasinglyAssemblyDic[assemblyName.Name].Contains(typeName))
                    {
                        increasinglyAssemblyDic[assemblyName.Name].Add(typeName);
                    }
                    var properties = type.GetProperties();
                    //获取组件的属性，如果属性是Unity对象，则再获取一次属性
                    foreach (var propertyInfo in properties)
                    {
                        var propertyInfoAssemblyName = propertyInfo.PropertyType.Assembly.GetName().Name;
                        var propertyInfoTypeName = propertyInfo.PropertyType.FullName;
                        if (typeName.StartsWith("UnityEngine") || typeName.StartsWith("TMPro"))
                        {
                            if (!increasinglyAssemblyDic.ContainsKey(propertyInfoAssemblyName))
                            {
                                increasinglyAssemblyDic.Add(propertyInfoAssemblyName, new List<string>());
                            }
                            if (!increasinglyAssemblyDic[propertyInfoAssemblyName].Contains(propertyInfoTypeName))
                            {
                                increasinglyAssemblyDic[propertyInfoAssemblyName].Add(propertyInfoTypeName);
                            }
                        }
                        if (propertyInfo.PropertyType.BaseType == typeof(UnityEngine.Object))
                        {
                            //为了确保大部分类都被获取到，直接获取组件的属性类
                            foreach (var property in propertyInfo.PropertyType.GetProperties())
                            {
                                var propertyType = property.PropertyType.GetType();
                                if (property.PropertyType.IsArray)
                                {
                                    propertyType = property.PropertyType.GetElementType();
                                }
                                propertyInfoAssemblyName = propertyType.Assembly.GetName().Name;
                                propertyInfoTypeName = propertyType.FullName;
                                if (typeName.StartsWith("UnityEngine") || typeName.StartsWith("TMPro"))
                                {
                                    if (!increasinglyAssemblyDic.ContainsKey(propertyInfoAssemblyName))
                                    {
                                        increasinglyAssemblyDic.Add(propertyInfoAssemblyName, new List<string>());
                                    }
                                    if (!increasinglyAssemblyDic[propertyInfoAssemblyName].Contains(propertyInfoTypeName))
                                    {
                                        increasinglyAssemblyDic[propertyInfoAssemblyName].Add(propertyInfoTypeName);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            count++;
            EditorUtility.DisplayProgressBar("Find Class", pfb.name, count / (float)asstIds.Length);
        }
        
        EditorUtility.DisplayProgressBar("Progress", "ReadLink.xml", 0);
        string filePath = @$"{Application.dataPath}\HybridCLRData\Generated\link.xml";

        var data = File.ReadAllText(filePath);

        XmlDocument xml = new XmlDocument();
        xml.LoadXml(data);
        XmlNode linker = xml.SelectSingleNode(xml.DocumentElement.Name);
        XmlNodeList assemblyList = linker.ChildNodes;

        Dictionary<string, List<string>> assemblyDic = new Dictionary<string, List<string>>();
        count = 0;
        foreach (var typeListItem in assemblyList)
        {
            var typeListElement = (XmlElement)typeListItem;
            var assemblyNmae = typeListElement.GetAttribute("fullname");
            if (!assemblyDic.ContainsKey(assemblyNmae))
            {
                assemblyDic.Add(assemblyNmae, new List<string>());
            }
            var typeListNodeList = (XmlNode)typeListItem;
            foreach (var typeItem in typeListNodeList.ChildNodes)
            {
                var typeElement = (XmlElement)typeItem;
                var typeName = typeElement.GetAttribute("fullname");
                if (!assemblyDic[assemblyNmae].Contains(typeName))
                {
                    assemblyDic[assemblyNmae].Add(typeName);
                }
                count++;
                EditorUtility.DisplayProgressBar("Find Class", typeName, count / (float)typeListNodeList.ChildNodes.Count);
            }
        }

        foreach (var assemblyName in increasinglyAssemblyDic.Keys)
        {
            if (!assemblyDic.ContainsKey(assemblyName))
            {
                var assemblyNode = xml.CreateElement(linker.FirstChild.Name);
                assemblyNode.SetAttribute("fullname", assemblyName);
                assemblyDic.Add(assemblyName, increasinglyAssemblyDic[assemblyName]);
                foreach (var typeName in increasinglyAssemblyDic[assemblyName])
                {
                    var typeNode = xml.CreateElement(linker.FirstChild.FirstChild.Name);
                    typeNode.SetAttribute("fullname", typeName);
                    typeNode.SetAttribute("preserve", "all");
                    assemblyNode.AppendChild(typeNode);
                }
                linker.AppendChild(assemblyNode);
                continue;
            }
            foreach (var typeName in increasinglyAssemblyDic[assemblyName])
            {
                if (!assemblyDic[assemblyName].Contains(typeName))
                {
                    var typeNode = xml.CreateElement(linker.FirstChild.FirstChild.Name);
                    typeNode.SetAttribute("fullname", typeName);
                    typeNode.SetAttribute("preserve", "all");
                    //assemblyNode.AppendChild(typeNode);
                    foreach (XmlElement assemblyElement in assemblyList)
                    {
                        if (assemblyElement.GetAttribute("fullname") == assemblyName)
                        {
                            assemblyElement.AppendChild(typeNode);
                        }
                    }
                }
            }
        }
        xml.Save($"{Application.dataPath}/link.xml");
        EditorUtility.ClearProgressBar();
        AssetDatabase.Refresh();
    }

    public static void GetUnityAssembly(object[] objects,ref Dictionary<string, List<string>> dic)
    {
        foreach (var obj in objects)
        {
            var type = obj.GetType();
            string typeName = type.FullName;
            var assemblyName = type.Assembly.GetName();
            if (typeName.StartsWith("UnityEngine") || typeName.StartsWith("TMPro"))
            {
                if (!dic.ContainsKey(assemblyName.Name))
                {
                    dic.Add(assemblyName.Name, new List<string>());
                }
                if (!dic[assemblyName.Name].Contains(typeName))
                {
                    dic[assemblyName.Name].Add(typeName);
                }
            }
        }
    }
    //[UnityEditor.UnityEditor.MenuItem("整合工具/读取XML测试")]
    public static void ReadXML()
    {
        string filePath = @$"{Application.dataPath}\HybridCLRData\Generated\link.xml";
        var data = File.ReadAllText(filePath);
        XmlDocument xml = new XmlDocument();
        xml.LoadXml(data);
        XmlNode linker = xml.SelectSingleNode(xml.DocumentElement.Name);
        XmlNodeList assemblyList = linker.ChildNodes;
        Debug.Log(linker.FirstChild.Name);
        //var testElement = xml.CreateElement(linker.FirstChild.Name);
        //testElement.SetAttribute("fullname", "test");
        //var subTestElement = xml.CreateElement(linker.FirstChild.FirstChild.Name);
        //subTestElement.SetAttribute("fullname", "subtest");
        //subTestElement.SetAttribute("preserve", "all");
        //testElement.AppendChild(subTestElement);
        //linker.AppendChild(testElement);
        foreach (var typeListItem in assemblyList)
        {
            var typeListElement = (XmlElement)typeListItem;
            Debug.Log(typeListElement.Name);
            Debug.Log($"{typeListElement.GetAttribute("fullname")} Assembly");
            var typeListNodeList = (XmlNode)typeListItem;
            foreach (var typeItem in typeListNodeList.ChildNodes)
            {
                var typeElement = (XmlElement)typeItem;
                Debug.Log($"{typeElement.GetAttribute("fullname")} Type");
            }
        }
        var testElement = xml.CreateElement(linker.FirstChild.Name);
        testElement.SetAttribute("fullname", "test");
        var subTestElement = xml.CreateElement(linker.FirstChild.FirstChild.Name);
        subTestElement.SetAttribute("fullname", "subtest");
        subTestElement.SetAttribute("preserve", "all");
        testElement.AppendChild(subTestElement);
        linker.FirstChild.AppendChild(testElement);
        xml.Save($"{Application.dataPath}/Test.xml");
        AssetDatabase.Refresh();
    }
    //[UnityEditor.UnityEditor.MenuItem("整合工具/读取预制体测试")]
    public static void ReadPrefabs()
    {
        EditorUtility.DisplayProgressBar("Progress", "Find Class...", 0);
        string[] dirs = { "Assets/HotUpdateAssets" };
        var asstIds = AssetDatabase.FindAssets("t:Prefab", dirs);
        int count = 0;
        List<string> classList = new List<string>();
        Dictionary<string, List<string>> assemblyDic = new Dictionary<string, List<string>>();
        for (int i = 0; i < asstIds.Length; i++)
        {
            string path = AssetDatabase.GUIDToAssetPath(asstIds[i]);
            var pfb = AssetDatabase.LoadAssetAtPath<GameObject>(path);

            var coms = pfb.GetComponentsInChildren<Component>();
            foreach (var com in coms)
            {
                var type = com.GetType();
                var assemblyName = type.Assembly.GetName();
                var properties = type.GetProperties();
                foreach (var propertyInfo in properties)
                {
                    if (propertyInfo.PropertyType.BaseType == typeof(UnityEngine.Object))
                    {
                        Debug.Log(propertyInfo.PropertyType);
                        foreach (var testype in propertyInfo.PropertyType.GetProperties())
                        {
                            Debug.Log(testype.PropertyType);
                            if (testype.PropertyType.IsArray)
                            {
                                Debug.Log(testype.PropertyType.GetElementType());
                                Debug.Log(testype.PropertyType.GetElementType().BaseType.Name);
                            }
                        }
                    }
                }

                string typeName = type.FullName;
                if ((typeName.StartsWith("UnityEngine") || typeName.StartsWith("TMPro")))
                {
                    if (!assemblyDic.ContainsKey(assemblyName.Name))
                    {
                        assemblyDic.Add(assemblyName.Name, new List<string>());
                    }
                    if (!assemblyDic[assemblyName.Name].Contains(typeName))
                    {
                        assemblyDic[assemblyName.Name].Add(typeName);
                    }
                }
            }
            count++;
            EditorUtility.DisplayProgressBar("Find Class", pfb.name, count / (float)asstIds.Length);
        }
        for (int i = 0; i < classList.Count; i++)
        {
            classList[i] = string.Format("<type fullname=\"{0}\" preserve=\"all\"/>", classList[i]);
        }
        EditorUtility.ClearProgressBar();
        Debug.Log("完成读取预制体");
    }
}