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
                case HybridBuildOption.BuildScript:
                case HybridBuildOption.BuildApplication:
                case HybridBuildOption.BuildAll:
                    if (CheckScriptPathExsist())
                    {
                        Debug.unityLogger.Log($"CheckScriptPathExsist Success");
                    }
                    else
                    {
                        Debug.unityLogger.LogError("CheckScriptPathExsist", $"CheckScriptPathExsist Failed");
                        return;
                    }

                    break;
            }
            StartBuild();
        }

        /// <summary>
        /// 确认是否存在AOT补充Dll路径和HotUpdatePath路径
        /// </summary>
        bool CheckScriptPathExsist()
        {
            bool hasPatchedAOTDLLPath = false;
            bool hasHotUpdateDllPath = false;

            var patchedAOTDLLPathGUID = AssetDatabase.AssetPathToGUID(_hybridBuilderSetting.PatchedAOTDLLCollectPath);
            var hotUpdateDLLPathGUID = AssetDatabase.AssetPathToGUID(_hybridBuilderSetting.HotUpdateDLLCollectPath);

            var buildPackage = AssetBundleCollectorSettingData.Setting.GetPackage(buildPackageName);
            foreach (var group in buildPackage.Groups)
            {
                foreach (var collector in group.Collectors)
                {
                    Debug.unityLogger.Log($"正在遍历CollectorPath ==> {collector.CollectorGUID}");
                    if (string.Equals(patchedAOTDLLPathGUID, collector.CollectorGUID))
                    {
                        hasPatchedAOTDLLPath = true;
                        Debug.unityLogger.Log($"hasPatchedAOTDLLPath == CollectorGUID ==> {patchedAOTDLLPathGUID}");
                    }
                    else if (string.Equals(hotUpdateDLLPathGUID, collector.CollectorGUID))
                    {
                        hasHotUpdateDllPath = true;
                        Debug.unityLogger.Log($"hasPatchedAOTDLLPath == CollectorGUID ==> {collector.CollectorGUID}");
                    }
                }
            }

            return hasPatchedAOTDLLPath && hasHotUpdateDllPath;
        }
        
        void StartBuild()
        {
            HybridScriptableBuildParameters buildParameters = new HybridScriptableBuildParameters();
            buildParameters.PatchedAOTDLLCollectPath = _hybridBuilderSetting.PatchedAOTDLLCollectPath;
            buildParameters.HotUpdateDLLCollectPath = _hybridBuilderSetting.HotUpdateDLLCollectPath;
            buildParameters.BuildOutputRoot = _hybridBuilderSetting.buildOutputPath;
            buildParameters.HybridBuildOption = _hybridBuilderSetting.hybridBuildOption;

            //打包后的拷贝目录,有需求可以自行更改
            buildParameters.BuildinFileRoot = AssetBundleBuilderHelper.GetStreamingAssetsRoot();
            buildParameters.BuildPipeline = BuildPipeline.ToString();
            buildParameters.BuildBundleType = (int) EBuildBundleType.AssetBundle;
            buildParameters.BuildTarget = BuildTarget;
            buildParameters.PackageName = PackageName;
            buildParameters.PackageVersion = GetPackageVersion();
            buildParameters.EnableSharePackRule = true;
            buildParameters.VerifyBuildingResult = true;
            buildParameters.FileNameStyle =  _hybridBuilderSetting.assetFileNameStyle;
            buildParameters.BuildinFileCopyOption = _hybridBuilderSetting.assetBuildinFileCopyOption;
            buildParameters.BuildinFileCopyParams = _hybridBuilderSetting.assetBuildinFileCopyParams;
            buildParameters.CompressOption = _hybridBuilderSetting.assetCompressOption;
            buildParameters.ClearBuildCacheFiles = _hybridBuilderSetting.isClearBuildCache;
            buildParameters.UseAssetDependencyDB = _hybridBuilderSetting.isUseAssetDependDB;
            buildParameters.BuiltinShadersBundleName = GetBuiltinShaderBundleName();
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
            switch (_hybridBuilderSetting.hybridBuildOption)
            {
                case HybridBuildOption.BuildAll:
                    _hybridBuilderSetting.AssetBuildVersion++;
                    _hybridBuilderSetting.ScriptBuildVersion++;
                    break;
                case HybridBuildOption.BuildApplication:
                    _hybridBuilderSetting.AssetBuildVersion++;
                    _hybridBuilderSetting.ScriptBuildVersion++;
                    _hybridBuilderSetting.ReleaseBuildVersion++;
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