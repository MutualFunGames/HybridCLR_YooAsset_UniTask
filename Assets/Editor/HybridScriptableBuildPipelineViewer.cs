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
        
        public HybridScriptableBuildPipelineViewer(BuildTarget buildTarget,
            HybridBuilderSetting hybridBuilderSetting, VisualElement parent)
            : base(EBuildPipeline.ScriptableBuildPipeline, buildTarget, hybridBuilderSetting, parent)
        {
            _hybridBuilderSetting = hybridBuilderSetting;
        }

        /// <summary>
        /// 执行构建
        /// </summary>
        protected override void ExecuteBuild()
        {
            if (_hybridBuilderSetting.hybridBuildOption == HybridBuildOption.None)
            {
                return;
            }
            
            switch (_hybridBuilderSetting.hybridBuildOption)
            {
                case HybridBuildOption.BuildScript:
                    if (CheckScriptPathExsist())
                    {
                        Debug.unityLogger.Log($"CheckScriptPathExsist Success");
                    }
                    else
                    {
                        Debug.unityLogger.LogError("CheckScriptPathExsist", $"CheckScriptPathExsist Failed");
                        return;
                    }
                    StartBuild(false);
                    break;
                case HybridBuildOption.BuildApplication:
                    if (CheckScriptPathExsist())
                    {
                        Debug.unityLogger.Log($"CheckScriptPathExsist Success");
                    }
                    else
                    {
                        Debug.unityLogger.LogError("CheckScriptPathExsist", $"CheckScriptPathExsist Failed");
                        return;
                    }
                    StartBuild(false);
                    StartBuild(true);
                    BuildApplication();
                    break;
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
                    StartBuild(false);
                    StartBuild(true);
                    break;
                case HybridBuildOption.BuildAsset:
                    StartBuild(true);
                    break;
            }
            UpdateBuildVersion();
        }


        void BuildApplication()
        {
            var activeBuildTarget = EditorUserBuildSettings.activeBuildTarget;
            switch (activeBuildTarget)
            {
                case BuildTarget.Android:
                    BuildHelper.BuildAPK(_hybridBuilderSetting.buildOutputPath,_hybridBuilderSetting.GetApplicationBuildVersion(true));
                    break;
                case BuildTarget.StandaloneWindows:
                    break;
            }
        }

        /// <summary>
        /// 确认是否存在AOT补充Dll路径和HotUpdatePath路径
        /// </summary>
        bool CheckScriptPathExsist()
        {
            if (string.IsNullOrEmpty(_hybridBuilderSetting.ScriptPackageName))
            {
                Debug.unityLogger.LogError("CheckScriptPathExsist",$"ScriptPackageName == Null ");
                return false;
            }
            bool hasPatchedAOTDLLPath = false;
            bool hasHotUpdateDllPath = false;

            var patchedAOTDLLPathGUID = AssetDatabase.AssetPathToGUID(_hybridBuilderSetting.PatchedAOTDLLCollectPath);
            var hotUpdateDLLPathGUID = AssetDatabase.AssetPathToGUID(_hybridBuilderSetting.HotUpdateDLLCollectPath);

            var buildPackage = AssetBundleCollectorSettingData.Setting.GetPackage(_hybridBuilderSetting.ScriptPackageName);
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
        
        void StartBuild(bool isBuildAsset)
        {
            HybridScriptableBuildParameters buildParameters = new HybridScriptableBuildParameters();
            buildParameters.PatchedAOTDLLCollectPath = _hybridBuilderSetting.PatchedAOTDLLCollectPath;
            buildParameters.HotUpdateDLLCollectPath = _hybridBuilderSetting.HotUpdateDLLCollectPath;
            buildParameters.BuildOutputRoot = _hybridBuilderSetting.buildOutputPath;
            buildParameters.IsBuildAsset = isBuildAsset;

            //打包后的拷贝目录,有需求可以自行更改,建议不要设置StreamingAsset，会随包打出
            buildParameters.BuildinFileRoot = AssetBundleBuilderHelper.GetStreamingAssetsRoot();
            buildParameters.BuildPipeline = BuildPipeline.ToString();
            buildParameters.BuildBundleType = (int) EBuildBundleType.AssetBundle;
            buildParameters.BuildTarget = BuildTarget;
            var packageName=string.Empty;
            if (isBuildAsset)
            {
                packageName = _hybridBuilderSetting.AssetPackageName;
            }
            else
            {
                packageName = _hybridBuilderSetting.ScriptPackageName;
            }
            buildParameters.PackageName=packageName;
            buildParameters.PackageVersion = _hybridBuilderSetting.GetBuildVersions(isBuildAsset);
            buildParameters.EnableSharePackRule = true;
            buildParameters.VerifyBuildingResult = true;
            buildParameters.FileNameStyle =  _hybridBuilderSetting.assetFileNameStyle;
            buildParameters.BuildinFileCopyOption = _hybridBuilderSetting.assetBuildinFileCopyOption;
            buildParameters.BuildinFileCopyParams = _hybridBuilderSetting.assetBuildinFileCopyParams;
            buildParameters.CompressOption = _hybridBuilderSetting.assetCompressOption;
            buildParameters.ClearBuildCacheFiles = _hybridBuilderSetting.isClearBuildCache;
            buildParameters.UseAssetDependencyDB = _hybridBuilderSetting.isUseAssetDependDB;
            buildParameters.BuiltinShadersBundleName = GetBuiltinShaderBundleName(packageName);
            buildParameters.EncryptionServices = CreateEncryptionInstance();

            HybrdiScriptableBuildPipeline pipeline = new HybrdiScriptableBuildPipeline();
            var buildResult = pipeline.Run(buildParameters, true);
            if (buildResult.Success)
            {
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
        private string GetBuiltinShaderBundleName(string packageName)
        {
            var uniqueBundleName = AssetBundleCollectorSettingData.Setting.UniqueBundleName;
            var packRuleResult = DefaultPackRule.CreateShadersPackRuleResult();
            return packRuleResult.GetBundleName(packageName, uniqueBundleName);//todo
        }
    }
}
#endif