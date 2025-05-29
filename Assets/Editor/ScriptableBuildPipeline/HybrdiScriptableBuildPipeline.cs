using System;
using System.Collections.Generic;
using UnityEngine;
using YooAsset.Editor;

public class HybrdiScriptableBuildPipeline : IBuildPipeline
{
    public BuildResult Run(BuildParameters buildParameters, bool enableLog)
    {
        if (buildParameters is HybridScriptableBuildParameters)
        {
            var hybridBuildParameters = buildParameters as HybridScriptableBuildParameters;

            AssetBundleBuilder builder = new AssetBundleBuilder();
            return builder.Run(buildParameters, GetHybridBuildPipeline(hybridBuildParameters.HybridBuildOption),
                enableLog);   
        }
        else
        {
            throw new Exception($"Invalid build parameter type : {buildParameters.GetType().Name}");
        }
    }

    private List<IBuildTask> GetHybridBuildPipeline(HybridBuildOption hybridBuildOption)
    {
        List<IBuildTask> pipeline = new List<IBuildTask>();
        switch (hybridBuildOption)
        {
            case HybridBuildOption.BuildAsset:
                pipeline.AddRange(new List<IBuildTask>
                {
                    new TaskPrepare_SBP(),
                    new TaskGetBuildMap_SBP(),
                    new TaskBuilding_SBP(),
                    new TaskVerifyBuildResult_SBP(),
                    new TaskEncryption_SBP(),
                    new TaskUpdateBundleInfo_SBP(),
                    new TaskCreateManifest_SBP(),
                    new TaskCreateReport_SBP(),
                    new TaskCreatePackage_SBP(),
                    new TaskCopyBuildinFiles_SBP(),
                    new TaskCreateCatalog_SBP()
                });
                break;
            case HybridBuildOption.BuildScript:
                pipeline.Add(new TaskBuildScript_SBP());
                break;
            case HybridBuildOption.BuildAll:
                //如果需要同时构建资源和代码
                //需要确保代码在资源构建之前就已经在AssetBundle文件夹中
                pipeline.AddRange(new List<IBuildTask>
                {
                    new TaskPrepare_SBP(),
                    new TaskBuildScript_SBP(),
                    new TaskGetBuildMap_SBP(),
                    new TaskBuilding_SBP(),
                    new TaskVerifyBuildResult_SBP(),
                    new TaskEncryption_SBP(),
                    new TaskUpdateBundleInfo_SBP(),
                    new TaskCreateManifest_SBP(),
                    new TaskCreateReport_SBP(),
                    new TaskCreatePackage_SBP(),
                    new TaskCopyBuildinFiles_SBP(),
                    new TaskCreateCatalog_SBP()
                });
                break;
            case HybridBuildOption.BuildApplication:
                pipeline.AddRange(new List<IBuildTask>
                {
                    new TaskPrepare_SBP(),
                    new TaskBuildScript_SBP(),
                    new TaskGetBuildMap_SBP(),
                    new TaskBuilding_SBP(),
                    new TaskVerifyBuildResult_SBP(),
                    new TaskEncryption_SBP(),
                    new TaskUpdateBundleInfo_SBP(),
                    new TaskCreateManifest_SBP(),
                    new TaskCreateReport_SBP(),
                    new TaskCreatePackage_SBP(),
                    new TaskCopyBuildinFiles_SBP(),
                    new TaskCreateCatalog_SBP(),
                    new TaskBuildApplication_SBP()
                });
                break;
        }

        return pipeline;
    }
}