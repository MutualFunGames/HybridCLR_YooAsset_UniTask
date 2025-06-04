using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using UnityEditor;
using UnityEditor.UIElements;
using UnityEngine;
using UnityEngine.UIElements;
using YooAsset.Editor;


public class HybridBuilderWindow : EditorWindow
{
    private HybridBuilderSetting _hybridBuilderSetting;


    private Toolbar _toolbar;
    private ToolbarMenu _packageMenu;
    private ToolbarMenu _hybridBuilderSettingMenu;
    private VisualElement _container;

    [MenuItem("整合工具/Hybrid Builder", false, 102)]
    public static void OpenWindow()
    {
        HybridBuilderWindow window =
            GetWindow<HybridBuilderWindow>("Hybrid Builder", true, WindowsDefine.DockedWindowTypes);
        window.minSize = new Vector2(800, 600);
    }

    public void CreateGUI()
    {
        try
        {
            VisualElement root = this.rootVisualElement;

            // 加载布局文件
            var visualAsset = UxmlLoader.LoadWindowUXML<HybridBuilderWindow>();
            if (visualAsset == null)
                return;

            visualAsset.CloneTree(root);
            _toolbar = root.Q<Toolbar>("Toolbar");
            _container = root.Q("Container");
            

            var hybridBuilderSettings = FindAllHybridBuilderSettings();
            if (hybridBuilderSettings.Count == 0)
            {
                var label = new Label();
                label.text = "Not found any HybridBuilderSetting";
                label.style.width = 100;
                _toolbar.Add(label);
                return;
            }

            //HybridBuilder打包设置
            {
                _hybridBuilderSetting = hybridBuilderSettings[0];
                _hybridBuilderSettingMenu = new ToolbarMenu();
                _hybridBuilderSettingMenu.style.width = 200;
                foreach (var hybridBuilderSetting in hybridBuilderSettings)
                {
                    _hybridBuilderSettingMenu.menu.AppendAction(hybridBuilderSetting.name,
                        HybridBuilderSettingMenuAction, HybridBuilderSettingMenuFun, hybridBuilderSetting);
                }

                _toolbar.Add(_hybridBuilderSettingMenu);
            }
            RefreshBuildPipelineView();
        }
        catch (Exception e)
        {
            Debug.LogError(e.ToString());
        }
    }

    private void RefreshBuildPipelineView()
    {
        // 清空扩展区域
        _container.Clear();
        
        _hybridBuilderSettingMenu.text = _hybridBuilderSetting.name;

        var buildTarget = EditorUserBuildSettings.activeBuildTarget;

        var viewer =
            new HybridScriptableBuildPipelineViewer(buildTarget, _hybridBuilderSetting, _container);

    }

    /// <summary>
    /// 查找工程下所有HybridBuilderSetting类型文件
    /// </summary>
    /// <returns></returns>
    /// <exception cref="Exception"></exception>
    List<HybridBuilderSetting> FindAllHybridBuilderSettings()
    {
        var hybridBuilderSettings = new List<HybridBuilderSetting>();
        string[] guids = AssetDatabase.FindAssets("t:HybridBuilderSetting");
        if (guids.Length == 0)
            throw new System.Exception($"Not found any assets : {nameof(HybridBuilderSetting)}");

        foreach (string assetGUID in guids)
        {
            string assetPath = AssetDatabase.GUIDToAssetPath(assetGUID);
            var assetType = AssetDatabase.GetMainAssetTypeAtPath(assetPath);
            if (assetType == typeof(HybridBuilderSetting))
            {
                var hybridBuilderSetting = AssetDatabase.LoadAssetAtPath<HybridBuilderSetting>(assetPath);
                if (hybridBuilderSetting == null)
                {
                    throw new System.Exception($"LoadError : {assetPath}");
                }

                hybridBuilderSettings.Add(hybridBuilderSetting);
            }
        }

        return hybridBuilderSettings;
    }
    

    void HybridBuilderSettingMenuAction(DropdownMenuAction action)
    {
        var targetSetting = (HybridBuilderSetting) action.userData;
        if (_hybridBuilderSetting != targetSetting)
        {
            _hybridBuilderSetting = targetSetting;
            RefreshBuildPipelineView();
        }
    }

    private DropdownMenuAction.Status HybridBuilderSettingMenuFun(DropdownMenuAction action)
    {
        var targetSetting = (HybridBuilderSetting) action.userData;
        if (_hybridBuilderSetting == targetSetting)
            return DropdownMenuAction.Status.Checked;
        else
            return DropdownMenuAction.Status.Normal;
    }

    
}