using System.IO;
using UnityEngine;
using UnityEngine.Serialization;
using YooAsset.Editor;

public enum HybridBuildOption
{
    /// <summary>
    /// 构建热更新资产
    /// </summary>
    BuildAsset,
    /// <summary>
    /// 构建AOT以及热更新脚本
    /// 并复制到指定文件夹
    /// </summary>
    BuildScript,
    /// <summary>
    /// 构建AOT脚本
    /// 并复制到指定文件夹
    /// </summary>
    BuildAOTScript,
    /// <summary>
    /// 构建热更新脚本
    /// 并复制到指定文件夹
    /// </summary>
    BuildHotUpdateScript,
    /// <summary>
    /// 构建热更新资产与所有脚本
    /// </summary>
    BuildAssetAndScript,
    /// <summary>
    /// 构建资产与脚本,并打包可执行程序或APK
    /// </summary>
    BuildAll,
    /// <summary>
    /// 构建资产与脚本,并导出安卓工程
    /// </summary>
    ExportAndroidProject
}
[CreateAssetMenu(fileName = "HybridBuilderSettings", menuName = "Scriptable Objects/HybridBuilderSettings")]
public class HybridBuilderSetting : ScriptableObject
{
    void OnValidate()
    {
        if (string.IsNullOrEmpty(BuildOutputPath))
        {
            BuildOutputPath = AssetBundleBuilderHelper.GetDefaultBuildOutputRoot();
        }
    }
    
    /// <summary>
    /// 打包输出路径
    /// </summary>
    public string BuildOutputPath
    {
        get => _buildOutputPath;
        set
        {
            _buildOutputPath = value;
            HotUpdateAssetsOutputPath = Path.Combine(_buildOutputPath,"HotUpdateAssets");
            HotUpdateDllOutputPath = Path.Combine(_buildOutputPath,"HotUpdateDll");
        }
    }
    [SerializeField]
    private string _buildOutputPath;
    
    /// <summary>
    /// 热更新资源输出路径
    /// </summary>
    public string HotUpdateAssetsOutputPath;

    /// <summary>
    /// 热更新Dll输出路径
    /// </summary>
    public string HotUpdateDllOutputPath;

    /// <summary>
    /// 资源构建版本
    /// </summary>
    public int AssetBuildVersion = 0;

    /// <summary>
    /// 脚本构建版本
    /// </summary>
    public int ScriptBuildVersion = 0;

    /// <summary>
    /// 发行版本
    /// </summary>
    public int ReleaseBuildVersion = 0;
    
    /// <summary>
    /// 版本文件名
    /// </summary>
    public  string VersionFileName = "VERSION.txt";
    
    /// <summary>
    /// 是否使用自增版本
    /// </summary>
    public bool IsUseSelfIncrementingVersions = false;
    
    /// <summary>
    /// 混合构建选项
    /// </summary>
    public HybridBuildOption HybridBuildOption;

}