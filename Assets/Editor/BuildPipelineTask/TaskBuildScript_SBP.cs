using System.IO;
using HybridCLR.Editor.Commands;
using UnityEngine;
using YooAsset.Editor;

public class TaskBuildScript_SBP : IBuildTask
{
    public void Run(BuildContext context)
    {
        var buildParametersContext = context.GetContextObject<BuildParametersContext>();
        var buildParameters = buildParametersContext.Parameters as HybridScriptableBuildParameters;
        
        
        CompileDllCommand.CompileDllActiveBuildTarget();
        PrebuildCommand.GenerateAll();
        
        //获取需要补充元数据的Dll
        BuildHelper.GetPatchedAOTAssemblyListToHybridCLRSettings();

        var projectPath=Directory.GetParent(Application.dataPath).FullName;
        var pathcedAOTDllFullPath=Path.Combine(projectPath,buildParameters.PatchedAOTDLLCollectPath);
        BuildHelper.CopyPatchedAOTDllToCollectPath(pathcedAOTDllFullPath);
        
        var hotUpdateDLLFullPath=Path.Combine(projectPath,buildParameters.HotUpdateDLLCollectPath);
        BuildHelper.CopyHotUpdateDllToCollectPath(hotUpdateDLLFullPath);
    }
}
