using System.IO;
using HybridCLR.Editor.Commands;
using UnityEngine;
using YooAsset.Editor;

public class TaskBuildApplication_SBP : IBuildTask
{
    public void Run(BuildContext context)
    {
        var buildParametersContext = context.GetContextObject<BuildParametersContext>();
        var buildParameters = buildParametersContext.Parameters as HybridScriptableBuildParameters;
        
    }
}
