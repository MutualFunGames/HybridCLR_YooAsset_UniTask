using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using Cysharp.Threading.Tasks;
using HybridCLR;
using Newtonsoft.Json;
using UnityEngine;
using UniFramework.Event;
using YooAsset;

public class HybridLauncher : MonoBehaviour
{
    /// <summary>
    /// 资源系统运行模式
    /// </summary>
    public EPlayMode PlayMode = EPlayMode.EditorSimulateMode;

    public HybridRuntimeSettings RuntimeSettings;

    void Awake()
    {
        Debug.Log($"资源系统运行模式：{PlayMode}");
        Application.targetFrameRate = 60;
        Application.runInBackground = true;
        DontDestroyOnLoad(this.gameObject);
    }

    async UniTask Start()
    {
        if (!RuntimeSettings)
        {
            Debug.unityLogger.LogError("RuntimeSettings", "RuntimeSettings == Null");
            return;
        }

        // 游戏管理器
        GameManager.Instance.Behaviour = this;

        // 初始化事件系统
        UniEvent.Initalize();

        // 初始化资源系统
        YooAssets.Initialize();

        // 加载更新页面
        var go = Resources.Load<GameObject>("PatchWindow");
        GameObject.Instantiate(go);

        string[] packages = new string[]
            {
                RuntimeSettings.ScriptPackageName,
                RuntimeSettings.AssetPackageName
            }
            ;

        foreach (var package in packages)
        {
            // 开始补丁更新流程
            var operation = new PatchOperation(package,PlayMode, RuntimeSettings);
            YooAssets.StartOperation(operation);
            await operation;
        }
        
        var scriptPackage = YooAssets.GetPackage(RuntimeSettings.ScriptPackageName);
        if (scriptPackage.InitializeStatus != EOperationStatus.Succeed)
        {
            Debug.unityLogger.LogError("ScriptPackage", "InitializeStatus is Falied");
            return;
        }
        
        if (!await LoadMetadataForAOTAssemblies(scriptPackage))
        {
            Debug.unityLogger.LogError("LoadMetadataForAOTAssemblies", "Load Falied");
            return;
        }
        
        if (!await LoadHotUpdateAssemblies(scriptPackage))
        {
            Debug.unityLogger.LogError("LoadHotUpdateAssemblies", "Load Falied");
        }
        

        // 设置默认的资源包
        var gamePackage = YooAssets.GetPackage(RuntimeSettings.AssetPackageName);
        YooAssets.SetDefaultPackage(gamePackage);

        // 切换到主页面场景
        SceneEventDefine.ChangeToHomeScene.SendEventMessage();
    }

    public async UniTask<bool> LoadMetadataForAOTAssemblies(ResourcePackage scriptPackage)
    {
        HomologousImageMode mode = HomologousImageMode.SuperSet;
        
        var handle = scriptPackage.LoadRawFileSync("AOTDLLs");
        await handle;
        if (handle.Status != EOperationStatus.Succeed)
        {
            Debug.unityLogger.LogError("ScriptPackageName", $"AOTDLLs LoadRawFileSync {handle.LastError}");
            return false;
        }
        var data = handle.GetRawFileText();
        if (string.IsNullOrEmpty(data))
        {
            Debug.unityLogger.LogError("ScriptPackageName", "AOTDLLs is null or empty");
            return false;
        }
        var dllNames = JsonConvert.DeserializeObject<List<string>>(data);
        foreach (var name in dllNames)
        {
            var dataHandle = scriptPackage.LoadRawFileAsync(name);
            await dataHandle.ToUniTask();
            var dllData = dataHandle.GetRawFileData();
            if (dllData == null|| dllData.Length == 0)
            {
                Debug.unityLogger.LogError("ScriptPackageName", $"{name} is null or empty");
                continue;
            }
            // 加载assembly对应的dll，会自动为它hook。一旦aot泛型函数的native函数不存在，用解释器版本代码
            LoadImageErrorCode err = RuntimeApi.LoadMetadataForAOTAssembly(dllData, mode);
            Debug.unityLogger.Log($"LoadMetadataForAOTAssembly:{name}. mode:{mode} ret:{err}");
        }
        return true;
    }
    
    async UniTask<bool> LoadHotUpdateAssemblies(ResourcePackage scriptPackage)
    {
        var handle = scriptPackage.LoadRawFileSync("HotUpdateDLLs");
        await handle.ToUniTask();
        var data = handle.GetRawFileText();
        if (string.IsNullOrEmpty(data))
        {
            Debug.unityLogger.LogError("LoadHotUpdateAssemblies", "HotUpdateDLLs is null or empty");
            return false;
        }
        var dllNames = JsonConvert.DeserializeObject<List<string>>(data);
        foreach (var DllName in dllNames)
        {
            var dataHandle = scriptPackage.LoadRawFileAsync(DllName);
            await dataHandle.ToUniTask();
            if (dataHandle.Status != EOperationStatus.Succeed)
            {
                Debug.unityLogger.LogError("LoadHotUpdateAssemblies", $"资源加载失败 {DllName}");
                return false;
            }
            var dllData = dataHandle.GetRawFileData();
            if (dllData == null|| dllData.Length == 0)
            {
                Debug.unityLogger.LogError("LoadHotUpdateAssemblies", $"获取Dll数据失败 {DllName}");
                return false;
            }
            Assembly assembly = Assembly.Load(dllData);
            
            Debug.unityLogger.Log(assembly.GetTypes());
            Debug.unityLogger.Log($"加载热更新Dll:{DllName}");
        }
        return true;
    }
}