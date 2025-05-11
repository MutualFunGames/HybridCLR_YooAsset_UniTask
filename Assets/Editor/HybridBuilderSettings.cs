using UnityEngine;
using YooAsset.Editor;

[CreateAssetMenu(fileName = "HybridBuilderSettings", menuName = "Scriptable Objects/HybridBuilderSettings")]
public class HybridBuilderSettings : ScriptableObject
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
    public  string BuildOutputPath;

    /// <summary>
    /// 资源构建版本
    /// </summary>
    public int AssetBuildVersion = 0;

    /// <summary>
    /// 代码构建版本
    /// </summary>
    public int ScriptBuildVersion = 0;

    /// <summary>
    /// 是否使用自增版本
    /// </summary>
    public bool IsUseSelfIncrementingVersions = false;
}
