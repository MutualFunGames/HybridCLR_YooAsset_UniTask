using System.IO;
using HybridCLR.Editor;
using HybridCLR.Editor.Commands;
using UnityEditor;
using UnityEngine;
using YooAsset.Editor;

public class TaskBuildScript_SBP : IBuildTask
{
    public void Run(BuildContext context)
    {
        var buildParametersContext = context.GetContextObject<BuildParametersContext>();
        var buildParameters = buildParametersContext.Parameters as HybridScriptableBuildParameters;


        if (!BuildHelper.CheckAccessMissingMetadata())
        {
            //如果是生成代码，则只需要更新AOT和热更新代码即可
            Il2CppDefGeneratorCommand.GenerateIl2CppDef();
            //由于该方法中已经执行了生成热更新dll，因此无需重复执行生成热更新DLL
            LinkGeneratorCommand.GenerateLinkXml();
                
            //通过打包项目在Library下生成裁剪后的AOTDLL,并且将其复制到HybridCLRData目录下
            StripAOTDllCommand.GenerateStripedAOTDlls();
                    
            //获取需要补充元数据的AOTDLL列表
            BuildHelper.GetPatchedAOTAssemblyListToHybridCLRSettings();
        }
        else
        {
            CompileDllCommand.CompileDllActiveBuildTarget();
        }
        

        var projectPath = Directory.GetParent(Application.dataPath).FullName;
        var pathcedAOTDllFullPath = Path.Combine(projectPath, buildParameters.PatchedAOTDLLCollectPath);
        BuildHelper.CopyPatchedAOTDllToCollectPath(pathcedAOTDllFullPath);

        var hotUpdateDLLFullPath = Path.Combine(projectPath, buildParameters.HotUpdateDLLCollectPath);
        BuildHelper.CopyHotUpdateDllToCollectPath(hotUpdateDLLFullPath);
        
    }
}