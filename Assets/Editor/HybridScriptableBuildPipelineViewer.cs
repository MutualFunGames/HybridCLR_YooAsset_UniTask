#if UNITY_2019_4_OR_NEWER
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using HybridCLR.Editor.Commands;
using UnityEditor;
using UnityEngine;
using UnityEditor.UIElements;
using UnityEngine.UIElements;

namespace YooAsset.Editor
{
    internal class HybridScriptableBuildPipelineViewer : HybridBuildPipeViewerBase
    {
        private HybridBuilderSetting _hybridBuilderSetting;
        public HybridScriptableBuildPipelineViewer(string packageName, BuildTarget buildTarget,
            HybridBuilderSetting hybridBuilderSetting, VisualElement parent)
            : base(packageName, EBuildPipeline.ScriptableBuildPipeline, buildTarget, hybridBuilderSetting, parent)
        {
            _hybridBuilderSetting = hybridBuilderSetting;
        }

        /// <summary>
        /// 执行构建
        /// </summary>
        protected override void ExecuteBuild()
        {
            switch (_hybridBuilderSetting.hybridBuildOption)
            {
                case HybridBuildOption.BuildAll:
                    break;
                case HybridBuildOption.BuildAsset:
                    BuildAsset();
                    break;
            }
            
        }
        public void BuildAPK()
        {
            //先生成AOT文件，再进行打包，以确保所有引用库都被引用,废弃，因HybridCLR会修改构建管线，自动执行一次GenerateALL
            PrebuildCommand.GenerateAll();

            BuildPlayerOptions buildPlayerOptions = new BuildPlayerOptions();
            buildPlayerOptions.scenes = BuildHelper.GetBuildScenes();

            var versionString = _hybridBuilderSetting.GetBuildVersion();

            var buildPath = $"{_hybridBuilderSetting.buildOutputPath}{PlayerSettings.productName}_{versionString}_{DateTime.Now.ToString("yyyy_M_d_HH_mm_s")}";
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

            ScriptableBuildParameters buildParameters = new ScriptableBuildParameters();
            buildParameters.BuildOutputRoot = _hybridBuilderSetting.buildOutputPath;
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

            ScriptableBuildPipeline pipeline = new ScriptableBuildPipeline();
            var buildResult = pipeline.Run(buildParameters, true);
            if (buildResult.Success)
                EditorUtility.RevealInFinder(buildResult.OutputPackageDirectory);
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