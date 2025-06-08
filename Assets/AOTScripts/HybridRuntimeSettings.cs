using System;
using System.IO;
using UnityEngine;
using UnityEngine.Serialization;
using YooAsset;

[CreateAssetMenu(fileName = "HybridRuntimeSettings", menuName = "Scriptable Objects/HybridRuntimeSettings")]
public class HybridRuntimeSettings : ScriptableObject
{

    /// <summary>
    /// 资源包加密服务类
    /// </summary>
    public Type EncryptionServices;
    
    
    /// <summary>
    /// 资源服务器地址
    /// </summary>
    public string HostServerIP;



    /// <summary>
    /// 发行版本
    /// </summary>
    public int ReleaseBuildVersion;

    /// <summary>
    /// 资源构建版本
    /// </summary>
    public int AssetBuildVersion;

    /// <summary>
    /// 脚本构建版本
    /// </summary>
    public int ScriptBuildVersion;

    /// <summary>
    /// 代码包名
    /// </summary>
    public string ScriptPackageName;
    
    /// <summary>
    /// 资源包名
    /// </summary>
    public string AssetPackageName;
}