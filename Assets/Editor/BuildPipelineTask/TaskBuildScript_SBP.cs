using System.IO;
using HybridCLR.Editor.Commands;
using UnityEngine;
using YooAsset.Editor;

public class TaskBuildScript_SBP : IBuildTask
{
    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void Run(BuildContext context)
    {
        var buildParametersContext = context.GetContextObject<BuildParametersContext>();
        var buildParameters = buildParametersContext.Parameters as HybridScriptableBuildParameters;
        
        CompileDllCommand.CompileDllActiveBuildTarget();
        PrebuildCommand.GenerateAll();
        
        //获取需要补充元数据的Dll
        BuildHelper.GetPatchedAOTAssemblyListToHybridCLRSettings();

        string aotPackagePath = Path.Combine("");
        
        // foreach (var group in package.Groups)
        // {
        //     if (group.GroupName == AOTDLLGroupName)
        //     {
        //         foreach (var collector in group.Collectors)
        //         {
        //             aotDllOutputPath = collector.CollectPath;
        //         }
        //     }
        // }
        BuildHelper.CopyPatchedAOTDllToPackagePath(aotPackagePath);
    }
}
