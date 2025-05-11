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
using Cysharp.Threading.Tasks;
using YooAsset.Editor;
using YooAsset;
using Newtonsoft.Json;
using System.Xml;
using HybridCLR.Editor.Settings;

public class BuildHelper
{
    public static string BuildOutputPath=AssetBundleBuilderHelper.GetDefaultBuildOutputRoot();
    /// <summary>
    /// 工程目录路径，Assets上一层
    /// </summary>
    public static string ProjectPath = Application.dataPath.Replace("Assets", "");

    public static string DllPath = $"{ProjectPath}/HybridCLRData/HotUpdateDlls/Android/";

    public static string PackageExportPath = $"{ProjectPath}/BuildPacakage/";

    public static string HotUpdateAssetsPath = $"{Application.dataPath}/HotUpdateAssets/";

    public static string HotUpdateDllPath = $"{HotUpdateAssetsPath}HotUpdateDll/";
    /// <summary>
    /// 版本文件名
    /// </summary>
    public static string VersionFileName = "/VERSION.txt";

    /// <summary>
    /// 热更新配置的Group名称，用来查找热更新dll存放位置
    /// </summary>
    public static string HotFixDllGroupName = "HotUpdateDll";

    public static string AOTDLLGroupName = "AOT";
    // Start is called before the first frame update
    static string[] GetBuildScenes()
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
    public static void BuildAPK()
    {
        //先生成AOT文件，再进行打包，以确保所有引用库都被引用,废弃，因HybridCLR会修改构建管线，自动执行一次GenerateALL
        PrebuildCommand.GenerateAll();

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
    // [MenuItem("整合工具/打初始AB包")]
    // public static void BuildOriginAssetBundle()
    // {
    //     #region 获取资源版本
    //     CreateVersionFile();
    //     var versionString = File.ReadAllText(Application.streamingAssetsPath + VersionFileName);
    //     //var versionStringArray = versionString.Split(".");
    //     //var version = int.Parse(String.Join("", versionStringArray));
    //
    //     #endregion
    //
    //     #region 打包DLL并重命名,放入热更新文件夹作为RawFile打包
    //     GenerateAOTDllListFile();
    //     BuildAndCopyHotUpdateDll();
    //     #endregion
    //
    //     #region 
    //     //version++;
    //     string defaultOutputRoot = AssetBundleBuilderHelper.GetDefaultBuildOutputRoot();
    //     BuildParameters buildParameters = new ScriptableBuildParameters();
    //     buildParameters.BuildOutputRoot = defaultOutputRoot;
    //     buildParameters.BuildTarget = EditorUserBuildSettings.activeBuildTarget;
    //     buildParameters.BuildPipeline = AssetBundleBuilderSetting.GetPackageBuildPipeline();
    //     buildParameters.BuildMode = EBuildMode.ForceRebuild;
    //     buildParameters.PackageName = AssetBundleBuilderSettingData.Setting.BuildPackage;
    //
    //     //versionString = version + "";
    //     //var length = versionString.Length * 2 - 1;
    //     //for (int i = 1; i < length; i += 2)
    //     //{
    //     //    versionString = versionString.Insert(i, ".");
    //     //}
    //
    //     buildParameters.PackageVersion = versionString;
    //     buildParameters.VerifyBuildingResult = true;
    //     buildParameters.ShareAssetPackRule = new DefaultShareAssetPackRule();
    //     var esct = GetEncryptionServicesClassTypes();
    //     buildParameters.EncryptionServices = (IEncryptionServices)Activator.CreateInstance(esct[0]);
    //     buildParameters.CompressOption = AssetBundleBuilderSettingData.Setting.CompressOption;
    //     buildParameters.OutputNameStyle = AssetBundleBuilderSettingData.Setting.OutputNameStyle;
    //     buildParameters.CopyBuildinFileOption = AssetBundleBuilderSettingData.Setting.CopyBuildinFileOption;
    //     buildParameters.CopyBuildinFileTags = AssetBundleBuilderSettingData.Setting.CopyBuildinFileTags;
    //     if (AssetBundleBuilderSettingData.Setting.BuildPipeline == EBuildPipeline.BuiltinBuildPipeline)
    //     {
    //         buildParameters.SBPParameters = new BuildParameters.SBPBuildParameters();
    //         buildParameters.SBPParameters.WriteLinkXML = true;
    //     }
    //
    //     var builder = new AssetBundleBuilder();
    //     var buildResult = builder.Run(buildParameters);
    //     if (buildResult.Success)
    //     {
    //         File.WriteAllText(Application.streamingAssetsPath + VersionFileName, versionString);
    //         Debug.Log("初始AB包打完");
    //         EditorUtility.RevealInFinder(buildResult.OutputPackageDirectory);
    //     }
    //     #endregion
    // }
    // [MenuItem("整合工具/打增量AB包")]
    // public static void BuildIncreasinglyAssetBundle()
    // {
    //     #region 获取资源版本
    //     var versionString = File.ReadAllText(Application.streamingAssetsPath + VersionFileName);
    //     var versionStringArray = versionString.Split(".");
    //     var version = int.Parse(String.Join("", versionStringArray));
    //
    //     #endregion
    //
    //     #region 打包DLL并重命名,放入热更新文件夹作为RawFile打包
    //     //GenerateAOTDllListFile();
    //     BuildAndCopyHotUpdateDll();
    //     #endregion
    //
    //     #region 
    //     version++;
    //     string defaultOutputRoot = AssetBundleBuilderHelper.GetDefaultOutputRoot();
    //     BuildParameters buildParameters = new BuildParameters();
    //     buildParameters.OutputRoot = defaultOutputRoot;
    //     buildParameters.BuildTarget = EditorUserBuildSettings.activeBuildTarget;
    //     buildParameters.BuildPipeline = AssetBundleBuilderSettingData.Setting.BuildPipeline;
    //     buildParameters.BuildMode = EBuildMode.IncrementalBuild;
    //     buildParameters.PackageName = AssetBundleBuilderSettingData.Setting.BuildPackage;
    //
    //     versionString = version + "";
    //     var length = versionString.Length * 2 - 1;
    //     for (int i = 1; i < length; i += 2)
    //     {
    //         versionString = versionString.Insert(i, ".");
    //     }
    //
    //     buildParameters.PackageVersion = versionString;
    //     buildParameters.VerifyBuildingResult = true;
    //     buildParameters.ShareAssetPackRule = new DefaultShareAssetPackRule();
    //     var esct = GetEncryptionServicesClassTypes();
    //     buildParameters.EncryptionServices = (IEncryptionServices)Activator.CreateInstance(esct[0]);
    //     buildParameters.CompressOption = AssetBundleBuilderSettingData.Setting.CompressOption;
    //     buildParameters.OutputNameStyle = AssetBundleBuilderSettingData.Setting.OutputNameStyle;
    //     buildParameters.CopyBuildinFileOption = AssetBundleBuilderSettingData.Setting.CopyBuildinFileOption;
    //     buildParameters.CopyBuildinFileTags = AssetBundleBuilderSettingData.Setting.CopyBuildinFileTags;
    //
    //     if (AssetBundleBuilderSettingData.Setting.BuildPipeline == EBuildPipeline.ScriptableBuildPipeline)
    //     {
    //         buildParameters.SBPParameters = new BuildParameters.SBPBuildParameters();
    //         buildParameters.SBPParameters.WriteLinkXML = true;
    //     }
    //
    //     var builder = new AssetBundleBuilder();
    //     var buildResult = builder.Run(buildParameters);
    //     if (buildResult.Success)
    //     {
    //         File.WriteAllText(Application.streamingAssetsPath + VersionFileName, versionString);
    //         Debug.Log("增量AB包打完");
    //         EditorUtility.RevealInFinder(buildResult.OutputPackageDirectory);
    //     }
    //     #endregion
    // }
    [MenuItem("整合工具/生成AOT补充文件并复制进文件夹")]
    public static void GenerateAOTDllListFile()
    {
        //先生成AOT文件，再进行打包，以确保所有引用库都被引用
        PrebuildCommand.GenerateAll();
    
        List<string> dllNames = new List<string>();
    
        var setting = HybridCLRSettings.LoadOrCreate();
        SerializedObject _serializedObject;
        _serializedObject = new SerializedObject(setting);
        var patchAOTAssemblies = _serializedObject.FindProperty("patchAOTAssemblies");
    
        foreach (SerializedProperty sp in patchAOTAssemblies)
        {
            dllNames.Add(sp.stringValue + ".dll");
        }
    
        var json = JsonConvert.SerializeObject(dllNames);
        var path = "";
    
        foreach (var package in AssetBundleCollectorSettingData.Setting.Packages)
        {
            foreach (var group in package.Groups)
            {
                if (group.GroupName == AOTDLLGroupName)
                {
                    foreach (var collector in group.Collectors)
                    {
                        path = collector.CollectPath;
                    }
                }
            }
        }
    
        var dllExportPath = SettingsUtil.GetAssembliesPostIl2CppStripDir(EditorUserBuildSettings.activeBuildTarget);
    
        Dictionary<string, byte[]> dllDatas = new Dictionary<string, byte[]>();
    
        foreach (var dllName in dllNames)
        {
            var dllPath = dllExportPath + "/" + dllName;
            if (!File.Exists(dllPath))
            {
                Debug.Log($"{dllName}不存在");
                continue;
            }
            var dllData = File.ReadAllBytes(dllPath);
            dllDatas.Add(dllName, dllData);
        }
    
        foreach (var dllName in dllDatas.Keys)
        {
            var dllPath = path + "/" + dllName + ".bytes";
            File.WriteAllBytes(dllPath, dllDatas[dllName]);
        }
    
        File.WriteAllText($"{path}/AOTDLLList.txt", json);
        AssetDatabase.Refresh();
        Debug.Log("AOT补充文件生成完毕");
    }
    [MenuItem("整合工具/生成热更新Dll并复制进文件夹")]
    public static void BuildAndCopyHotUpdateDll()
    {
        List<string> dllNames = new List<string>();
        var setting = HybridCLRSettings.LoadOrCreate();
        SerializedObject _serializedObject;
        _serializedObject = new SerializedObject(setting);
        var _hotUpdateAssemblyDefinitions = _serializedObject.FindProperty("hotUpdateAssemblyDefinitions");
        var _hotUpdateAssemblies = _serializedObject.FindProperty("hotUpdateAssemblies");
    
    
        foreach (SerializedProperty sp in _hotUpdateAssemblyDefinitions)
        {
            AssemblyDefinitionAsset ada = (AssemblyDefinitionAsset)sp.objectReferenceValue;
            dllNames.Add(ada.name + ".dll");
        }
        foreach (SerializedProperty sp in _hotUpdateAssemblies)
        {
            dllNames.Add(sp.stringValue + ".dll");
        }
        CompileDllCommand.CompileDllActiveBuildTarget();
    
        var dllExportPath = SettingsUtil.GetHotUpdateDllsOutputDirByTarget(EditorUserBuildSettings.activeBuildTarget);
    
        Dictionary<string, byte[]> dllDatas = new Dictionary<string, byte[]>();
        foreach (var dllName in dllNames)
        {
            var dllPath = dllExportPath + "/" + dllName;
            if (!File.Exists(dllPath))
            {
                Debug.Log($"{dllName}不存在");
                continue;
            }
            var dllData = File.ReadAllBytes(dllPath);
            dllDatas.Add(dllName, dllData);
        }
    
        foreach (var package in AssetBundleCollectorSettingData.Setting.Packages)
        {
            foreach (var group in package.Groups)
            {
                if (group.GroupName == HotFixDllGroupName)
                {
                    foreach (var collector in group.Collectors)
                    {
                        HotUpdateDllPath = collector.CollectPath;
                    }
                }
            }
        }
        var json = JsonConvert.SerializeObject(dllNames);
        File.WriteAllText($"{HotUpdateDllPath}/HotUpdateDLLList.txt", json);
        foreach (var dllName in dllDatas.Keys)
        {
            var dllPath = HotUpdateDllPath + "/" + dllName + ".bytes";
            File.WriteAllBytes(dllPath, dllDatas[dllName]);
        }
        AssetDatabase.Refresh();
        Debug.Log("生成热更新Dll成功");
    }
    private static List<Type> GetEncryptionServicesClassTypes()
    {
        return EditorTools.GetAssignableTypes(typeof(IEncryptionServices));
    }

    //[MenuItem("整合工具/删除本地沙盒文件夹")]
    public static void DeleteSandBoxDirectory()
    {
        var path = $"{ProjectPath}/SandBox";
        if (Directory.Exists(path))
        {
            Directory.Delete(path, true);
        }
        Debug.Log("沙盒文件夹删除成功");
    }
    [MenuItem("整合工具/删除本地AB包数据并重新创建版本文件")]
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
    //[MenuItem("整合工具/读取XML测试")]
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
    //[MenuItem("整合工具/读取预制体测试")]
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