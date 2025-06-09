using System;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Serialization;
using YooAsset;
using YooAsset.Editor;

public enum HybridBuildOption
{
    /// <summary>
    /// 不进行任何构建
    /// </summary>
    None,
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
    /// 构建资产与脚本
    /// 并打包应用程序
    /// </summary>
    BuildApplication,
}

[CreateAssetMenu(fileName = "HybridBuilderSettings", menuName = "Scriptable Objects/HybridBuilderSettings")]
public class HybridBuilderSettings : ScriptableObject
{
    void OnEnable()
    {
        if (string.IsNullOrEmpty(_buildOutputPath))
        {
            _buildOutputPath = AssetBundleBuilderHelper.GetDefaultBuildOutputRoot();
        }
    }
    
    public HybridRuntimeSettings RuntimeSettings;

    /// <summary>
    /// 资产包包名
    /// </summary>
    public string AssetPackageName
    {
        get => assetPackageName;
        set
        {
            if (string.IsNullOrEmpty(value))
            {
                return;
            }
            assetPackageName = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField] private string assetPackageName;


    /// <summary>
    /// 代码包包名
    /// </summary>
    public string ScriptPackageName
    {
        get => scriptPackageName;
        set
        {
            if (string.IsNullOrEmpty(value))
            {
                return;
            }
            scriptPackageName = value;
            EditorUtility.SetDirty(this);
        }
    }
    
    [SerializeField] private string scriptPackageName;
    
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

    [SerializeField] private string _buildOutputPath;

    public string GetBuildOutputPath()
    {
        return Path.Combine(buildOutputPath,_releaseBuildVersion.ToString());
    }

    /// <summary>
    /// 补充元数据AOTDLL路径 收集器组合 名称
    /// 在构建时,会遍历当前包中所有的 AssetBundleCollector 进行对比
    /// 如果没有将会报错
    /// </summary>
    public DefaultAsset PatchedAOTDLLFolder
    {
        get => _patchedAOTDLLFolder;
        set
        {
            _patchedAOTDLLFolder = value;
            EditorUtility.SetDirty(this);
        }
    }
    [SerializeField] private DefaultAsset _patchedAOTDLLFolder;

    public string PatchedAOTDLLCollectPath
    {
        get
        {
            if (!_patchedAOTDLLFolder){
                Debug.unityLogger.LogError("路径为空！",
                    $"PatchedAOTDLLFolder ===> {_patchedAOTDLLFolder} ");
                return String.Empty;
            }
            var patchedAOTDLLPath = AssetDatabase.GetAssetPath(_patchedAOTDLLFolder);
            return patchedAOTDLLPath;
        }
    }
    
    /// <summary>
    /// 热更新Dll路径 收集器组合 名称
    /// 在构建时,会遍历当前包中的所有的 AssetBundleCollector 进行对比
    /// 如果没有将会报错
    /// </summary>
    public DefaultAsset HotUpdateDLLFolder
    {
        get => _hotUpdateDLLFolder;
        set
        {
            _hotUpdateDLLFolder = value;

            EditorUtility.SetDirty(this);
        }
    } 
    [SerializeField] private DefaultAsset _hotUpdateDLLFolder;

    public string HotUpdateDLLCollectPath
    {
        get
        {
            if (!_hotUpdateDLLFolder){
                Debug.unityLogger.LogError("路径为空！",
                    $"HotUpdateDLLFolder ===> {_hotUpdateDLLFolder} ");
                return String.Empty;
            }
            var hotUpdateDLLPath = AssetDatabase.GetAssetPath(_hotUpdateDLLFolder);
            return hotUpdateDLLPath;
        }
    }
    /// <summary>
    /// 发行版本
    /// </summary>
    [SerializeField] private int _releaseBuildVersion = 0;

    public int ReleaseBuildVersion
    {
        get => _releaseBuildVersion;
        set
        {
            _releaseBuildVersion = value;
            EditorUtility.SetDirty(this);
        }
    }

    /// <summary>
    /// 资源构建版本
    /// </summary>
    [SerializeField] private int _assetBuildVersion;

    public int AssetBuildVersion
    {
        get => _assetBuildVersion;
        set
        {
            _assetBuildVersion = value;
            EditorUtility.SetDirty(this);
        }
    }

    /// <summary>
    /// 脚本构建版本
    /// </summary>
    [SerializeField] private int _scriptBuildVersion = 0;

    public int ScriptBuildVersion
    {
        get => _scriptBuildVersion;
        set
        {
            _scriptBuildVersion = value;
            EditorUtility.SetDirty(this);
        }
    }
    
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

    [SerializeField] private bool _isUseSelfIncrementingVersions;

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

    [SerializeField] private bool _isClearBuildCache;

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

    [SerializeField] private bool _isUseAssetDependDB;


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

    [SerializeField] private string _assetEncyptionClassName;


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

    [SerializeField] private ECompressOption _assetCompressOption;

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

    [SerializeField] private EFileNameStyle _assetFileNameStyle;


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

    [SerializeField] private EBuildinFileCopyOption _assetBuildinFileCopyOption;

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

    [SerializeField] private string _assetBuildinFileCopyParams;

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

    [SerializeField] private HybridBuildOption _hybridBuildOption;


    public string GetCurrentVersion(bool isBuild)
    {
        var buildVersion = string.Empty;
        if (isBuild)
        {
            buildVersion =
                $"{_releaseBuildVersion}_{_assetBuildVersion}_{_scriptBuildVersion}";
        }
        else
        {
            buildVersion =
                $"Realse:{_releaseBuildVersion} AssetPakcage:{_assetBuildVersion} ScriptPackge:{_scriptBuildVersion}";
        }

        return buildVersion;
    }
    
}