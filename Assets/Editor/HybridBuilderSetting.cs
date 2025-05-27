using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Serialization;
using YooAsset;
using YooAsset.Editor;

public enum HybridBuildOption
{
    /// <summary>
    /// 构建热更新资产与所有脚本
    /// </summary>
    BuildAll,
    
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
    BuildPatchedAOTScript,
    
    /// <summary>
    /// 构建热更新脚本
    /// 并复制到指定文件夹
    /// </summary>
    BuildHotUpdateScript,
    
    /// <summary>
    /// 构建资产与脚本
    /// 并打包APK
    /// </summary>
    BuildAPK,
    
    /// <summary>
    /// 构建资产与脚本,并导出安卓工程
    /// </summary>
    BuildAllAndExportAndroidProject
}
[CreateAssetMenu(fileName = "HybridBuilderSettings", menuName = "Scriptable Objects/HybridBuilderSettings")]
public class HybridBuilderSetting : ScriptableObject
{
    void OnEnable()
    {
        if (string.IsNullOrEmpty(_buildOutputPath))
        {
            _buildOutputPath = AssetBundleBuilderHelper.GetDefaultBuildOutputRoot();
        }
    }

    /// <summary>
    /// 打包输出路径
    /// </summary>
    public string buildOutputPath
    {
        get => _buildOutputPath;
        set
        {
            if (string.IsNullOrEmpty(value))
            {
                return;
            }
            _buildOutputPath = value;
            EditorUtility.SetDirty(this);
        }
    }

    [SerializeField]
    private string _buildOutputPath;

    /// <summary>
    /// 资源构建版本
    /// </summary>
    public int assetBuildVersion;


    /// <summary>
    /// 脚本构建版本
    /// </summary>
    public int scriptBuildVersion = 0;
    
    /// <summary>
    /// 发行版本
    /// </summary>
    public int releaseBuildVersion = 0;
    
    /// <summary>
    /// 版本文件名
    /// </summary>
    public  string versionFileName = "VERSION.txt";

    /// <summary>
    /// 是否使用自增版本
    /// </summary>
    public bool isUseSelfIncrementingVersions
    {
        get => _isUseSelfIncrementingVersions;
        set
        {
            _isUseSelfIncrementingVersions = value;
            EditorUtility.SetDirty(this);
        }
    }

    [SerializeField]
    private bool _isUseSelfIncrementingVersions;

    /// <summary>
    /// 是否清除构建缓存
    /// 当不勾选此项的时候，引擎会开启增量打包模式，会极大提高构建速度！ 
    /// </summary>
    public bool isClearBuildCache
    {
        get => _isClearBuildCache;
        set
        {
            _isClearBuildCache = value;
            EditorUtility.SetDirty(this);
        }
    }
    
    [SerializeField]
    private bool _isClearBuildCache;

    /// <summary>
    /// 在资源收集过程中，使用资源依赖关系数据库。
    /// 当开启此项的时候，会极大提高构建速度！
    /// </summary>
    public bool isUseAssetDependDB
    {
        get => _isUseAssetDependDB;
        set
        {
            _isUseAssetDependDB = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField]
    private  bool _isUseAssetDependDB;


    /// <summary>
    /// AB包加密方式
    /// </summary>
    public string assetEncyptionClassName
    {
        get => _assetEncyptionClassName;
        set
        {
            if (string.IsNullOrEmpty(value))
            {
                return;
            }

            _assetEncyptionClassName = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField]
    private string _assetEncyptionClassName;

    
    /// <summary>
    /// AB包压缩方式
    /// </summary>
    public ECompressOption assetCompressOption
    {
        get => _assetCompressOption;
        set
        {
            _assetCompressOption = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField]
    private ECompressOption _assetCompressOption;

    /// <summary>
    /// AB包命名方式
    /// </summary>
    public EFileNameStyle assetFileNameStyle
    {
        get => _assetFileNameStyle;
        set
        {
            _assetFileNameStyle = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField]
    private EFileNameStyle _assetFileNameStyle;


    /// <summary>
    /// 首包copy选项
    /// </summary>
    public EBuildinFileCopyOption assetBuildinFileCopyOption
    {
        get => _assetBuildinFileCopyOption;
        set
        {
            _assetBuildinFileCopyOption = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField]
    private EBuildinFileCopyOption _assetBuildinFileCopyOption;

    /// <summary>
    /// copy选项参数
    /// </summary>
    public string assetBuildinFileCopyParams
    {
        get => _assetBuildinFileCopyParams;
        set
        {
            if (string.IsNullOrEmpty(value))
            {
                return;
            }
            _assetBuildinFileCopyParams = value;
            EditorUtility.SetDirty(this);
        }
    }

    [SerializeField]
    private string _assetBuildinFileCopyParams;
    
    /// <summary>
    /// 混合构建选项
    /// </summary>
    public HybridBuildOption hybridBuildOption
    
    {
        get => _hybridBuildOption;
        set
        {
            _hybridBuildOption = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField]
    private HybridBuildOption _hybridBuildOption;
    

    public string GetBuildVersion()
    {
        var buildVersion =
            $"{releaseBuildVersion}_{assetBuildVersion}_{scriptBuildVersion}";
        return buildVersion;
    }
}