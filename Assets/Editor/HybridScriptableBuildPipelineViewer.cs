#if UNITY_2019_4_OR_NEWER
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using HybridCLR.Editor.Commands;
using Newtonsoft.Json;
using UnityEditor;
using UnityEngine;
using UnityEngine.UIElements;

namespace YooAsset.Editor
{
    internal class HybridScriptableBuildPipelineViewer : HybridBuildPipeViewerBase
    {
        private HybridBuilderSettings _hybridBuilderSettings;

        public HybridScriptableBuildPipelineViewer(BuildTarget buildTarget,
            HybridBuilderSettings hybridBuilderSettings, VisualElement parent)
            : base(EBuildPipeline.ScriptableBuildPipeline, buildTarget, hybridBuilderSettings, parent)
        {
            _hybridBuilderSettings = hybridBuilderSettings;
        }

        /// <summary>
        /// 执行构建
        /// </summary>
        protected override void ExecuteBuild()
        {
            if (_hybridBuilderSettings.hybridBuildOption == HybridBuildOption.None)
            {
                return;
            }

            switch (_hybridBuilderSettings.hybridBuildOption)
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
                    BuildApplication();
                    StartBuild(false);
                    StartBuild(true);
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
                    EditorUtility.RevealInFinder(_hybridBuilderSettings. GetBuildOutputPath());
                    break;
                case HybridBuildOption.BuildAsset:
                    StartBuild(true);
                    break;
            }
            var json = JsonConvert.SerializeObject(_hybridBuilderSettings.RuntimeSettings);
            File.WriteAllText(Path.Combine(_hybridBuilderSettings. buildOutputPath, "RuntimeSettings.json"), json);
        }


        void BuildApplication()
        {
            var activeBuildTarget = EditorUserBuildSettings.activeBuildTarget;
            
            switch (activeBuildTarget)
            {
                case BuildTarget.Android:
                    BuildHelper.BuildAPK(_hybridBuilderSettings.GetBuildOutputPath(),
                        _hybridBuilderSettings.GetCurrentVersion(true));
                    break;
                case BuildTarget.StandaloneWindows:
                    break;
            }
                                                     
            //为了保证一次打包所有的包Release版本一致，应该在打完所有包之后增加Release版本
            _hybridBuilderSettings.ReleaseBuildVersion++;
            _hybridBuilderSettings.RuntimeSettings.ReleaseBuildVersion =
                _hybridBuilderSettings.ReleaseBuildVersion;
            EditorUtility.SetDirty(_hybridBuilderSettings.RuntimeSettings);
                    
            EditorUtility.RevealInFinder(_hybridBuilderSettings. GetBuildOutputPath());
        }

        /// <summary>
        /// 确认是否存在AOT补充Dll路径和HotUpdatePath路径
        /// </summary>
        bool CheckScriptPathExsist()
        {
            if (string.IsNullOrEmpty(_hybridBuilderSettings.ScriptPackageName))
            {
                Debug.unityLogger.LogError("CheckScriptPathExsist", $"ScriptPackageName == Null ");
                return false;
            }

            bool hasPatchedAOTDLLPath = false;
            bool hasHotUpdateDllPath = false;

            var patchedAOTDLLPathGUID = AssetDatabase.AssetPathToGUID(_hybridBuilderSettings.PatchedAOTDLLCollectPath);
            var hotUpdateDLLPathGUID = AssetDatabase.AssetPathToGUID(_hybridBuilderSettings.HotUpdateDLLCollectPath);

            var buildPackage =
                AssetBundleCollectorSettingData.Setting.GetPackage(_hybridBuilderSettings.ScriptPackageName);
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
            buildParameters.PatchedAOTDLLCollectPath = _hybridBuilderSettings.PatchedAOTDLLCollectPath;
            buildParameters.HotUpdateDLLCollectPath = _hybridBuilderSettings.HotUpdateDLLCollectPath;
            buildParameters.BuildOutputRoot = _hybridBuilderSettings. GetBuildOutputPath();
            buildParameters.IsBuildAsset = isBuildAsset;

            //打包后的拷贝目录,有需求可以自行更改,建议不要设置StreamingAsset，会随包打出
            buildParameters.BuildinFileRoot = AssetBundleBuilderHelper.GetStreamingAssetsRoot();
            buildParameters.BuildTarget = BuildTarget;
            if (isBuildAsset)
            {
                buildParameters.PackageName = _hybridBuilderSettings.AssetPackageName;
                buildParameters.BuiltinShadersBundleName = GetBuiltinShaderBundleName(_hybridBuilderSettings.AssetPackageName);
                buildParameters.BuildPipeline = BuildPipeline.ToString();
                buildParameters.BuildBundleType = (int) EBuildBundleType.AssetBundle;
                buildParameters.PackageVersion = _hybridBuilderSettings.AssetBuildVersion.ToString();
            }
            else
            {
                buildParameters.PackageName = _hybridBuilderSettings.ScriptPackageName;
                buildParameters.BuildBundleType = (int)EBuildBundleType.RawBundle;
                buildParameters.BuildPipeline = nameof(EBuildPipeline.RawFileBuildPipeline);
                buildParameters.PackageVersion = _hybridBuilderSettings.ScriptBuildVersion.ToString();
            }
            
            buildParameters.EnableSharePackRule = true;
            buildParameters.VerifyBuildingResult = true;
            buildParameters.FileNameStyle = _hybridBuilderSettings.assetFileNameStyle;
            buildParameters.BuildinFileCopyOption = _hybridBuilderSettings.assetBuildinFileCopyOption;
            buildParameters.BuildinFileCopyParams = _hybridBuilderSettings.assetBuildinFileCopyParams;
            buildParameters.CompressOption = _hybridBuilderSettings.assetCompressOption;
            buildParameters.ClearBuildCacheFiles = _hybridBuilderSettings.isClearBuildCache;
            buildParameters.UseAssetDependencyDB = _hybridBuilderSettings.isUseAssetDependDB;
            buildParameters.EncryptionServices = CreateEncryptionInstance();

            HybrdiScriptableBuildPipeline pipeline = new HybrdiScriptableBuildPipeline();
            var buildResult = pipeline.Run(buildParameters, true);
            if (buildResult.Success)
            {
                if (isBuildAsset)
                {
                    _hybridBuilderSettings.RuntimeSettings.AssetPackageName = _hybridBuilderSettings.AssetPackageName;
                    _hybridBuilderSettings.RuntimeSettings.AssetBuildVersion = _hybridBuilderSettings.AssetBuildVersion;
                    _hybridBuilderSettings.AssetBuildVersion++;
                }
                else
                {
                    _hybridBuilderSettings.RuntimeSettings.ScriptPackageName = _hybridBuilderSettings.ScriptPackageName;
                    _hybridBuilderSettings.RuntimeSettings.ScriptBuildVersion =
                        _hybridBuilderSettings.ScriptBuildVersion;
                    _hybridBuilderSettings.ScriptBuildVersion++;
                }

                EditorUtility.SetDirty(_hybridBuilderSettings.RuntimeSettings);

                switch (_hybridBuilderSettings.hybridBuildOption)
                {
                    case HybridBuildOption.BuildAsset:
                    case HybridBuildOption.BuildScript:
                        EditorUtility.RevealInFinder(buildResult.OutputPackageDirectory);
                        break;
                }

                _hybridBuilderSettings.RuntimeSettings.EncryptionServices = buildParameters.EncryptionServices.GetType();
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
            return packRuleResult.GetBundleName(packageName, uniqueBundleName); //todo
        }
    }
}
#endif