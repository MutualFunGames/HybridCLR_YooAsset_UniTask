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
            AssetBundleBuilder builder = new AssetBundleBuilder();
            return builder.Run(buildParameters, GetHybridBuildPipeline(), enableLog);
        }
        else
        {
            throw new Exception($"Invalid build parameter type : {buildParameters.GetType().Name}");
        }
    }
    private List<IBuildTask> GetHybridBuildPipeline()
    {
        List<IBuildTask> pipeline = new List<IBuildTask>
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
        };
        return pipeline;
    }
}
