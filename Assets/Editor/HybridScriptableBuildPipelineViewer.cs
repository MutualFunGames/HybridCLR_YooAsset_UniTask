#if UNITY_2019_4_OR_NEWER
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using HybridCLR.Editor.Commands;
using UnityEditor;
using UnityEngine;
using UnityEngine.UIElements;

namespace YooAsset.Editor
{
    internal class HybridScriptableBuildPipelineViewer : HybridBuildPipeViewerBase
    {
        private HybridBuilderSetting _hybridBuilderSetting;

        private string buildPackageName;

        public HybridScriptableBuildPipelineViewer(string packageName, BuildTarget buildTarget,
            HybridBuilderSetting hybridBuilderSetting, VisualElement parent)
            : base(packageName, EBuildPipeline.ScriptableBuildPipeline, buildTarget, hybridBuilderSetting, parent)
        {
            _hybridBuilderSetting = hybridBuilderSetting;
            buildPackageName = packageName;
        }

        /// <summary>
        /// 执行构建
        /// </summary>
        protected override void ExecuteBuild()
        {
            switch (_hybridBuilderSetting.hybridBuildOption)
            {
                case HybridBuildOption.BuildAll:
                    CheckScriptPath();
                    break;
                case HybridBuildOption.BuildAsset:
                    BuildAsset();
                    break;
            }
        }

        /// <summary>
        /// 确认是否存在AOT补充Dll路径和HotUpdatePath路径
        /// </summary>
        void CheckScriptPath()
        {
            if (!_hybridBuilderSetting.PatchedAOTDLLFolder || !_hybridBuilderSetting.HotUpdateDLLFolder)
            {
                Debug.unityLogger.LogError("路径为空！",
                    $"PatchedAOTDLLFolder ===> {_hybridBuilderSetting.PatchedAOTDLLFolder}  PatchedAOTDLLFolder ===> {_hybridBuilderSetting.HotUpdateDLLFolder}");
                return;
            }

            var buildPackage = AssetBundleCollectorSettingData.Setting.GetPackage(buildPackageName);
            var patchedAOTDllPath = AssetDatabase.GetAssetPath(_hybridBuilderSetting.PatchedAOTDLLFolder);
            var patchedAOTDLLPathGUID = AssetDatabase.AssetPathToGUID(patchedAOTDllPath);


            Debug.unityLogger.Log($"获取AOT路径 ===> {patchedAOTDllPath}  AOT路径GUID ===> {patchedAOTDLLPathGUID}");
            foreach (var group in buildPackage.Groups)
            {
                foreach (var collector in group.Collectors)
                {
                    Debug.unityLogger.Log($"正在遍历CollectorPath ==> {collector.CollectorGUID}");
                    if (string.Equals(patchedAOTDLLPathGUID, collector.CollectorGUID))
                    {
                        Debug.unityLogger.Log($"找到GUID相同文件夹 ==> {patchedAOTDLLPathGUID}");
                    }
                }
            }
        }

        public void BuildAPK()
        {
            //先生成AOT文件，再进行打包，以确保所有引用库都被引用,废弃，因HybridCLR会修改构建管线，自动执行一次GenerateALL
            PrebuildCommand.GenerateAll();

            BuildPlayerOptions buildPlayerOptions = new BuildPlayerOptions();
            buildPlayerOptions.scenes = BuildHelper.GetBuildScenes();

            var versionString = _hybridBuilderSetting.GetBuildVersion();

            var buildPath =
                $"{_hybridBuilderSetting.buildOutputPath}{PlayerSettings.productName}_{versionString}_{DateTime.Now.ToString("yyyy_M_d_HH_mm_s")}";
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
            UnityEditor.BuildPipeline.BuildPlayer(buildPlayerOptions);

            EditorUtility.ClearProgressBar();
        }

        void BuildHotUpdateScript()
        {
        }

        void BuildAsset()
        {
            var fileNameStyle = _hybridBuilderSetting.assetFileNameStyle;
            var buildinFileCopyOption = _hybridBuilderSetting.assetBuildinFileCopyOption;
            var buildinFileCopyParams = _hybridBuilderSetting.assetBuildinFileCopyParams;
            var compressOption = _hybridBuilderSetting.assetCompressOption;
            var clearBuildCache = _hybridBuilderSetting.isClearBuildCache;
            var useAssetDependencyDB = _hybridBuilderSetting.isUseAssetDependDB;
            var builtinShaderBundleName = GetBuiltinShaderBundleName();

            HybridScriptableBuildParameters buildParameters = new HybridScriptableBuildParameters();
            buildParameters.BuildOutputRoot = _hybridBuilderSetting.buildOutputPath;
            //拷贝目录,有需求可以自行更改
            buildParameters.BuildinFileRoot = AssetBundleBuilderHelper.GetStreamingAssetsRoot();
            buildParameters.BuildPipeline = BuildPipeline.ToString();
            buildParameters.BuildBundleType = (int) EBuildBundleType.AssetBundle;
            buildParameters.BuildTarget = BuildTarget;
            buildParameters.PackageName = PackageName;
            buildParameters.PackageVersion = GetPackageVersion();
            buildParameters.EnableSharePackRule = true;
            buildParameters.VerifyBuildingResult = true;
            buildParameters.FileNameStyle = fileNameStyle;
            buildParameters.BuildinFileCopyOption = buildinFileCopyOption;
            buildParameters.BuildinFileCopyParams = buildinFileCopyParams;
            buildParameters.CompressOption = compressOption;
            buildParameters.ClearBuildCacheFiles = clearBuildCache;
            buildParameters.UseAssetDependencyDB = useAssetDependencyDB;
            buildParameters.BuiltinShadersBundleName = builtinShaderBundleName;
            buildParameters.EncryptionServices = CreateEncryptionInstance();

            HybrdiScriptableBuildPipeline pipeline = new HybrdiScriptableBuildPipeline();
            var buildResult = pipeline.Run(buildParameters, true);
            if (buildResult.Success)
            {
                UpdateBuildVersion();
                EditorUtility.RevealInFinder(buildResult.OutputPackageDirectory);
            }
        }

        void UpdateBuildVersion()
        {
            _hybridBuilderSetting.ReleaseBuildVersion++;
            switch (_hybridBuilderSetting.hybridBuildOption)
            {
                case HybridBuildOption.BuildAll:
                case HybridBuildOption.BuildAllAndExportAndroidProject:
                case HybridBuildOption.BuildAPK:
                    _hybridBuilderSetting.AssetBuildVersion++;
                    _hybridBuilderSetting.ScriptBuildVersion++;
                    break;
                case HybridBuildOption.BuildAsset:
                    _hybridBuilderSetting.AssetBuildVersion++;
                    break;
                case HybridBuildOption.BuildScript:
                    _hybridBuilderSetting.ScriptBuildVersion++;
                    break;
            }
        }

        /// <summary>
        /// 内置着色器资源包名称
        /// 注意：和自动收集的着色器资源包名保持一致！
        /// </summary>
        private string GetBuiltinShaderBundleName()
        {
            var uniqueBundleName = AssetBundleCollectorSettingData.Setting.UniqueBundleName;
            var packRuleResult = DefaultPackRule.CreateShadersPackRuleResult();
            return packRuleResult.GetBundleName(PackageName, uniqueBundleName);
        }
    }
}
#endif