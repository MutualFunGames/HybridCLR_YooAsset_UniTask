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


        switch (buildParameters.HybridBuildOption)
        {
            case HybridBuildOption.BuildApplication:
                //如果是打应用程序，则需要重新生成link以及宏命令
                PrebuildCommand.GenerateAll();
                break;
            case HybridBuildOption.BuildAll:
            case HybridBuildOption.BuildScript:
                //如果是生成代码，则只需要更新AOT和热更新代码即可
                Il2CppDefGeneratorCommand.GenerateIl2CppDef();
                LinkGeneratorCommand.GenerateLinkXml();
                StripAOTDllCommand.GenerateStripedAOTDlls();
                break;
        }

        CompileDllCommand.CompileDllActiveBuildTarget();

        //获取需要补充元数据的Dll
        BuildHelper.GetPatchedAOTAssemblyListToHybridCLRSettings();

        var projectPath = Directory.GetParent(Application.dataPath).FullName;
        var pathcedAOTDllFullPath = Path.Combine(projectPath, buildParameters.PatchedAOTDLLCollectPath);
        BuildHelper.CopyPatchedAOTDllToCollectPath(pathcedAOTDllFullPath);

        var hotUpdateDLLFullPath = Path.Combine(projectPath, buildParameters.HotUpdateDLLCollectPath);
        BuildHelper.CopyHotUpdateDllToCollectPath(hotUpdateDLLFullPath);

        //补全热更新预制体依赖
        BuildHelper.SupplementPrefabDependent();
    }
}