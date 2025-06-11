using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using System;
using HybridCLR.Editor.Commands;
using HybridCLR.Editor;
using System.IO;
using System.Linq;
using System.Xml;
using HybridCLR.Editor.AOT;
using HybridCLR.Editor.HotUpdate;
using HybridCLR.Editor.Meta;
using Newtonsoft.Json;
using UnityEditor.Build.Reporting;


public class BuildHelper
{
    /// <summary>
    /// 工程目录路径，Assets上一层
    /// </summary>
    public static string ProjectPath = Directory.GetParent(Application.dataPath).FullName;

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

    /// <summary>
    /// 一般来说，发布热更新包时，由于中间可能调用过generate/all，SettingsUtil.GetAssembliesPostIl2CppStripDir(target)目录中包含了最新的aot dll，
    /// 肯定无法检查出类型或者函数裁剪的问题。
    /// 需要在构建完主包后，将当时的aot dll保存下来，供后面补充元数据或者裁剪检查。
    ///  换句话说,验证一定要在打包之前,或者Generate/AotDlls之前执行
    /// </summary>
    /// <returns></returns>
    [MenuItem("HybridTool/验证元数据是否需要补充")]
    public static bool CheckAccessMissingMetadata()
    {
        BuildTarget target = EditorUserBuildSettings.activeBuildTarget;


        string aotDir = SettingsUtil.GetAssembliesPostIl2CppStripDir(target);

        // 第2个参数hotUpdateAssNames为热更新程序集列表。对于旗舰版本，该列表需要包含DHE程序集，即SettingsUtil.HotUpdateAndDHEAssemblyNamesIncludePreserved。
        var checker = new MissingMetadataChecker(aotDir, SettingsUtil.HotUpdateAssemblyNamesIncludePreserved);

        string hotUpdateDir = SettingsUtil.GetHotUpdateDllsOutputDirByTarget(target);
        bool notAnyMissing = false;
        foreach (var dll in SettingsUtil.HotUpdateAssemblyFilesExcludePreserved)
        {
            string dllPath = $"{hotUpdateDir}/{dll}";
            notAnyMissing = checker.Check(dllPath);
            if (!notAnyMissing)
            {
                Debug.unityLogger.LogError("验证元数据",$"验证{dll}失败");
                return false;
            }
        }

        return true;
    }
    [MenuItem("HybridTool/打APK包")]
    public static void Debug_BuildAPK()
    {
        var sampleOutputPath = Path.Combine(ProjectPath, "Bundles");
        BuildAPK(sampleOutputPath, "9999");
    }

    /// <summary>
    /// 打包安卓平台
    /// </summary>
    /// <param name="outputPath">  APK/Project输出路径  </param>
    /// <param name="isExportProject">  是否导出AndroidProject  </param>
    public static void BuildAPK(string outputPath, string version, bool isExportProject = false)
    {
        //如果是生成代码，则只需要更新AOT和热更新代码即可
        Il2CppDefGeneratorCommand.GenerateIl2CppDef();
        //由于该方法中已经执行了生成热更新dll，因此无需重复执行生成热更新DLL
        LinkGeneratorCommand.GenerateLinkXml();
            
        //补全热更新预制体依赖
        BuildHelper.SupplementPrefabDependent();
        
        BuildPlayerOptions buildPlayerOptions = new BuildPlayerOptions();
        buildPlayerOptions.scenes = GetBuildScenes();
        EditorUserBuildSettings.exportAsGoogleAndroidProject = isExportProject;
        var buildPath = string.Empty;

        buildPlayerOptions.target = BuildTarget.Android;
        if (isExportProject)
        {
            buildPath = Path.Combine(outputPath,
                $"{PlayerSettings.productName}_{version}_{DateTime.Now.ToString("yyyy_M_d_HH_mm_s")}");

            buildPlayerOptions.options = BuildOptions.None;
        }
        else
        {
            buildPath = Path.Combine(outputPath,
                $"{PlayerSettings.productName}_{version}_{DateTime.Now.ToString("yyyy_M_d_HH_mm_s")}.apk");
            buildPlayerOptions.options = BuildOptions.None;
        }

        Debug.unityLogger.Log(buildPath);
        buildPlayerOptions.locationPathName = buildPath;
        //执行打包 场景名字，打包路径
        var report=  BuildPipeline.BuildPlayer(buildPlayerOptions);
        
        if (report.summary.result == BuildResult.Succeeded)
        {
            //获取需要补充元数据的AOTDLL列表
            //这里为什么不执行Generate/AOTGenericReference和Generate/AotDlls
            //因为前者本身就是生成用于参考的文件,和下面这个方法一致
            //为什么不执行Generate/AotDlls,是因为执行AOT本质上就是打一次包并把裁剪后的AOTDLL拷贝到HybridCLRData目录下
            //因此如果要打包,直接在打包后通过TaskBuildScript_SBP将AOTDLL拷贝到Package目录即可
            BuildHelper.GetPatchedAOTAssemblyListToHybridCLRSettings();
                            
            //生成桥接函数
            MethodBridgeGeneratorCommand.GenerateMethodBridgeAndReversePInvokeWrapper();
            
            //以上是必须要在打包Application时完成的方法
        }

        EditorUtility.ClearProgressBar();
    }

    /// <summary>
    /// 获取AOT之前,应先编译热更新代码
    /// 执行之前需要先编译热更新代码 CompileDllCommand.CompileDllActiveBuildTarget()
    ///
    /// 通过对比热更新DLL和AOTDLL,获取需要补充元数据的AOTDLL
    /// 并将结果写入HybridCLRSettings中
    /// </summary>
    public static void GetPatchedAOTAssemblyListToHybridCLRSettings()
    {
        var gs = SettingsUtil.HybridCLRSettings;
        List<string> hotUpdateDllNames = SettingsUtil.HotUpdateAssemblyNamesExcludePreserved;

        AssemblyReferenceDeepCollector collector = new AssemblyReferenceDeepCollector(
            MetaUtil.CreateHotUpdateAndAOTAssemblyResolver(EditorUserBuildSettings.activeBuildTarget,
                hotUpdateDllNames), hotUpdateDllNames);
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
            var patchtedAOTAssemblysName = module.Name.Replace(".dll", string.Empty);
            Debug.Log($"需要补充元数据的AOT ========= {patchtedAOTAssemblysName}");
            patchtedAOTAssemblys.Add(patchtedAOTAssemblysName);
        }

        gs.patchAOTAssemblies = patchtedAOTAssemblys.ToArray();
    }

    [MenuItem("HybridTool/获取需要补充元数据的Dll")]
    public static void Debug_GetPatchedAOTAssemblyList()
    {
        CompileDllCommand.CompileDllActiveBuildTarget();

        GetPatchedAOTAssemblyListToHybridCLRSettings();
    }

    public static List<string> CopyDllFileToByte(string[] originFileNames, string originDir, string targetDir)
    {
        List<string> bytesFiles = new List<string>();
        foreach (var originFileName in originFileNames)
        {
            var dllFilePath = Path.Combine(ProjectPath, originDir, $"{originFileName}.dll");
            if (!File.Exists(dllFilePath))
            {
                Debug.Log($"{dllFilePath}不存在");
                continue;
            }

            var targetFileName = $"{originFileName}.bytes";
            var dllRawFilePath = Path.Combine(targetDir, targetFileName);
            File.Copy(dllFilePath, dllRawFilePath, true);
            bytesFiles.Add(originFileName);
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
            Debug.unityLogger.LogError("CopyPatchedAOTDllToCollectPath", $"{nameof(rawFileCollectPath)}===>Null");
            return;
        }

        var patchedAOTAssemblies = SettingsUtil.HybridCLRSettings.patchAOTAssemblies;

        var dllOutputPath = SettingsUtil.GetAssembliesPostIl2CppStripDir(EditorUserBuildSettings.activeBuildTarget);

        var dllRawFileAssetNames = CopyDllFileToByte(patchedAOTAssemblies, dllOutputPath, rawFileCollectPath);

        if (dllRawFileAssetNames != null && dllRawFileAssetNames.Count > 0)
        {
            var namesJson = JsonConvert.SerializeObject(dllRawFileAssetNames);
            File.WriteAllText($"{rawFileCollectPath}/AOTDLLs.txt", namesJson);
            AssetDatabase.Refresh();
            Debug.unityLogger.Log("CopyPatchedAOTDllToCollectPath Success!");
        }
        else
        {
            Debug.unityLogger.LogError("CopyPatchedAOTDllToCollectPath", $"{nameof(dllRawFileAssetNames)}===>Null");
        }
    }

    [MenuItem("HybridTool/生成AOT补充文件并复制进文件夹")]
    public static void Debug_GenerateAOTDllListFile()
    {
        //先生成AOT文件
        Il2CppDefGeneratorCommand.GenerateIl2CppDef();
        LinkGeneratorCommand.GenerateLinkXml();
        StripAOTDllCommand.GenerateStripedAOTDlls();

        var aotDllRawFileCollectPath = Path.Combine(Application.dataPath, "HotUpdateAssets", "PatchedAOTDLL");

        Debug.unityLogger.Log(aotDllRawFileCollectPath);
        CopyPatchedAOTDllToCollectPath(aotDllRawFileCollectPath);
    }

    [MenuItem("HybridTool/生成热更新Dll并复制进文件夹")]
    public static void Debug_GenerateHotUpdateDllListFile()
    {
        CompileDllCommand.CompileDllActiveBuildTarget();

        var hotUpdateDllRawFileCollectPath = Path.Combine(Application.dataPath, "HotUpdateAssets", "HotUpdateDLL");

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
            Debug.unityLogger.LogError("CopyHotUpdateDllToCollectPath", $"{nameof(rawFileCollectPath)}===>Null");
            return;
        }

        var hotUpdateAssemblies = SettingsUtil.HotUpdateAssemblyNamesExcludePreserved;

        var hotUpdateOutputPath =
            SettingsUtil.GetHotUpdateDllsOutputDirByTarget(EditorUserBuildSettings.activeBuildTarget);

        var dllRawFileAssetNames =
            CopyDllFileToByte(hotUpdateAssemblies.ToArray(), hotUpdateOutputPath, rawFileCollectPath);

        if (dllRawFileAssetNames != null && dllRawFileAssetNames.Count > 0)
        {
            var json = JsonConvert.SerializeObject(dllRawFileAssetNames);
            File.WriteAllText(Path.Combine(rawFileCollectPath, "HotUpdateDLLs.txt"), json);
            AssetDatabase.Refresh();
            Debug.unityLogger.Log("CopyHotUpdateDllToCollectPath  Success");
        }
        else
        {
            Debug.unityLogger.LogError("CopyHotUpdateDllToCollectPath", $"{nameof(dllRawFileAssetNames)}===>Null");
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

    [MenuItem("HybridTool/补全热更新预制体依赖")]
    public static void Debug_SupplementPrefabDependent()
    {
        EditorUtility.DisplayProgressBar("Progress", "Find Class...", 0);
        SupplementPrefabDependent();
        EditorUtility.ClearProgressBar();
    }

    public static void SupplementPrefabDependent()
    {
        string[] dirs = {"Assets/HotUpdateAssets"};
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
                        type = propertyInfo.PropertyType;
                        typeName = type.FullName;
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

                                    if (!increasinglyAssemblyDic[propertyInfoAssemblyName]
                                            .Contains(propertyInfoTypeName))
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
            EditorUtility.DisplayProgressBar("Find Class", pfb.name, count / (float) asstIds.Length);
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
            var typeListElement = (XmlElement) typeListItem;
            var assemblyNmae = typeListElement.GetAttribute("fullname");
            if (!assemblyDic.ContainsKey(assemblyNmae))
            {
                assemblyDic.Add(assemblyNmae, new List<string>());
            }

            var typeListNodeList = (XmlNode) typeListItem;
            foreach (var typeItem in typeListNodeList.ChildNodes)
            {
                var typeElement = (XmlElement) typeItem;
                var typeName = typeElement.GetAttribute("fullname");
                if (!assemblyDic[assemblyNmae].Contains(typeName))
                {
                    assemblyDic[assemblyNmae].Add(typeName);
                }

                count++;
                EditorUtility.DisplayProgressBar("Find Class", typeName,
                    count / (float) typeListNodeList.ChildNodes.Count);
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
        AssetDatabase.Refresh();
    }
    
}