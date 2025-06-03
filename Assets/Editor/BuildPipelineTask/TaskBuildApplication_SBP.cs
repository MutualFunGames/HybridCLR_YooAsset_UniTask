using System.IO;
using HybridCLR.Editor.Commands;
using UnityEditor;
using UnityEngine;
using YooAsset.Editor;

public class TaskBuildApplication_SBP : IBuildTask
{
    public void Run(BuildContext context)
    {
        var buildParametersContext = context.GetContextObject<BuildParametersContext>();
        var buildParameters = buildParametersContext.Parameters as HybridScriptableBuildParameters;


        if (buildParameters.HybridBuildOption != HybridBuildOption.BuildApplication)
        {
            return;
        }
        
        var activeBuildTarget = EditorUserBuildSettings.activeBuildTarget;
        switch (activeBuildTarget)
        {
            case BuildTarget.Android:
                BuildHelper.BuildAPK(buildParameters.BuildOutputRoot,buildParameters.PackageVersion);
                break;
            case BuildTarget.StandaloneWindows:
                break;
        }
    }
}
