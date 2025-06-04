#if UNITY_2019_4_OR_NEWER
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.UIElements;
using UnityEngine;

using UnityEngine.UIElements;
using Object = UnityEngine.Object;

namespace YooAsset.Editor
{
    internal abstract class HybridBuildPipeViewerBase
    {
        private const int StyleWidth = 400;
        private const int LabelMinWidth = 180;
        
        protected readonly BuildTarget BuildTarget;
        protected readonly EBuildPipeline BuildPipeline;
        protected TemplateContainer Root;

        private TextField _buildOutputField;
        private TextField _buildVersionField;
        private PopupField<Enum> _buildModeField;
        private PopupField<Type> _encryptionField;
        private EnumField _compressionField;
        private EnumField _outputNameStyleField;
        private EnumField _copyBuildinFileOptionField;
        private TextField _copyBuildinFileTagsField;
        private ObjectField _patchedAOTDLLFolderField;
        private ObjectField _hotUpdateDLLFolderField;
        private Toggle _clearBuildCacheToggle;
        private Toggle _useAssetDependencyDBToggle;

        private HybridBuilderSetting _hybridBuilderSetting;

        public HybridBuildPipeViewerBase( EBuildPipeline buildPipeline, BuildTarget buildTarget,
            HybridBuilderSetting hybridBuildSetting,
            VisualElement parent)
        {
            BuildTarget = buildTarget;
            BuildPipeline = buildPipeline;
            _hybridBuilderSetting = hybridBuildSetting;

            if (CreateView(parent))
            {
                RefreshScriptCollectorGourpNameView();
                RefreshBuildinFileCopyOptionView();                
            }
        }


        private bool CreateView(VisualElement parent)
        {
            // 加载布局文件
            var visualAsset = UxmlLoader.LoadWindowUXML<HybridBuildPipeViewerBase>();
            if (visualAsset == null)
                return false;

            Root = visualAsset.CloneTree();
            Root.style.flexGrow = 1f;
            parent.Add(Root);

            // 输出目录
            var assetOutputPar = Root.Q("AssetOutputPar");
            _buildOutputField = assetOutputPar.Q<TextField>("BuildOutput");
            _buildOutputField.SetValueWithoutNotify(_hybridBuilderSetting.buildOutputPath);
            _buildOutputField.SetEnabled(false);
            
            var buildOutputPathBrowserButton = assetOutputPar.Q<Button>("BrowseButton");
            buildOutputPathBrowserButton.clicked += () =>
            {
                var defaultPath=_hybridBuilderSetting.buildOutputPath;
                BrowserFolder(defaultPath, (selectPath) =>
                {
                    _hybridBuilderSetting.buildOutputPath = selectPath;
                    _buildOutputField.SetValueWithoutNotify(selectPath);
                });
            };
            


            var versionPar = Root.Q("VersionPar");
            var versionToggle = versionPar.Q<Toggle>("Toggle");
            versionToggle.SetValueWithoutNotify(_hybridBuilderSetting.isUseSelfIncrementingVersions);
            versionToggle.RegisterValueChangedCallback(OnVersionToggleChange);

            // 构建版本
            _buildVersionField = versionPar.Q<TextField>("BuildVersion");
            _buildVersionField.style.width = StyleWidth;
            if (_hybridBuilderSetting.isUseSelfIncrementingVersions)
            {
                _buildVersionField.SetValueWithoutNotify(_hybridBuilderSetting.GetApplicationBuildVersion(false));
            }
            else
            {
                _buildVersionField.SetValueWithoutNotify(GetDefaultPackageVersion());
            }

            _buildVersionField.SetEnabled(false);
            
            var packageErrorLabel = Root.Q("PackageErrorLabel");
            // 检测构建包裹
            var packageNames = GetBuildPackageNames();

            var hasTwoAndMorePakcage = packageNames.Count > 1;
            packageErrorLabel.visible= !hasTwoAndMorePakcage;
            // 构建包裹
            if(!hasTwoAndMorePakcage)
            {
                return false;
            }
            
            
            var assetBundlePackageContainer = Root.Q("AssetBundlePackageContainer");
            
            int assetPakcageIndex = 0;
            var assetPakcageOption = new PopupField<string>(packageNames, assetPakcageIndex);
            assetPakcageOption.label="Asset Package";
            assetPakcageOption.style.width = StyleWidth;
            assetPakcageOption.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.AssetPackageName = evt.newValue;
            });
            
            if (string.IsNullOrEmpty(_hybridBuilderSetting.AssetPackageName))
            {
                _hybridBuilderSetting.AssetPackageName = packageNames[0];
            }
            else
            {
                assetPakcageOption.SetValueWithoutNotify(_hybridBuilderSetting.AssetPackageName);
            }
            assetBundlePackageContainer.Add(assetPakcageOption);
            
            
            var scriptPackageOption = new PopupField<string>(packageNames, assetPakcageIndex);
            scriptPackageOption.label="Script Package";
            scriptPackageOption.style.width = StyleWidth;
            scriptPackageOption.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.ScriptPackageName = evt.newValue;
            });
            if (string.IsNullOrEmpty(_hybridBuilderSetting.ScriptPackageName))
            {
                _hybridBuilderSetting.ScriptPackageName = packageNames[0];
            }
            else
            {
                scriptPackageOption.SetValueWithoutNotify(_hybridBuilderSetting.ScriptPackageName);
            }
            assetBundlePackageContainer.Add(scriptPackageOption);

            //assetPackageOption.Init(packageOption);
            // _packageMenu = new ToolbarMenu();
            // _packageMenu.style.width = 200;
            // foreach (var packageName in packageNames)
            // {
            //     _packageMenu.menu.AppendAction(packageName, PackageMenuAction, PackageMenuFun, packageName);
            // }
            //
            // _toolbar.Add(_packageMenu);
            // 加密方法
            {
                var encryptionContainer = Root.Q("EncryptionContainer");
                var encryptionClassTypes = EditorTools.GetAssignableTypes(typeof(IEncryptionServices));
                if (encryptionClassTypes.Count > 0)
                {
                    var encyptionClassName = _hybridBuilderSetting.assetEncyptionClassName;
                    int defaultIndex = encryptionClassTypes.FindIndex(x => x.FullName.Equals(encyptionClassName));
                    if (defaultIndex < 0)
                        defaultIndex = 0;
                    _encryptionField = new PopupField<Type>(encryptionClassTypes, defaultIndex);
                    _encryptionField.label = "Encryption";
                    _encryptionField.style.width = StyleWidth;
                    _encryptionField.RegisterValueChangedCallback(evt =>
                    {
                        _hybridBuilderSetting.assetEncyptionClassName = _encryptionField.value.FullName;
                    });
                    encryptionContainer.Add(_encryptionField);
                }
                else
                {
                    _encryptionField = new PopupField<Type>();
                    _encryptionField.label = "Encryption";
                    _encryptionField.style.width = StyleWidth;
                    encryptionContainer.Add(_encryptionField);
                }
            }

            // 压缩方式选项
            var compressOption = _hybridBuilderSetting.assetCompressOption;
            _compressionField = Root.Q<EnumField>("Compression");
            _compressionField.Init(compressOption);
            _compressionField.SetValueWithoutNotify(compressOption);
            _compressionField.style.width = StyleWidth;
            _compressionField.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.assetCompressOption = (ECompressOption) _compressionField.value;
            });

            // 输出文件名称样式
            var fileNameStyle = _hybridBuilderSetting.assetFileNameStyle;
            _outputNameStyleField = Root.Q<EnumField>("FileNameStyle");
            _outputNameStyleField.Init(fileNameStyle);
            _outputNameStyleField.SetValueWithoutNotify(fileNameStyle);
            _outputNameStyleField.style.width = StyleWidth;
            _outputNameStyleField.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.assetFileNameStyle = (EFileNameStyle) _outputNameStyleField.value;
            });

            // 首包文件拷贝选项
            var buildinFileCopyOption = _hybridBuilderSetting.assetBuildinFileCopyOption;
            _copyBuildinFileOptionField = Root.Q<EnumField>("CopyBuildinFileOption");
            _copyBuildinFileOptionField.Init(buildinFileCopyOption);
            _copyBuildinFileOptionField.SetValueWithoutNotify(buildinFileCopyOption);
            _copyBuildinFileOptionField.style.width = StyleWidth;
            _copyBuildinFileOptionField.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.assetBuildinFileCopyOption =
                    (EBuildinFileCopyOption) _copyBuildinFileOptionField.value;
                RefreshBuildinFileCopyOptionView();
            });

            // 首包文件拷贝参数
            var buildinFileCopyParams = _hybridBuilderSetting.assetBuildinFileCopyParams;
            _copyBuildinFileTagsField = Root.Q<TextField>("CopyBuildinFileParam");
            _copyBuildinFileTagsField.SetValueWithoutNotify(buildinFileCopyParams);
            _copyBuildinFileTagsField.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.assetBuildinFileCopyParams = _copyBuildinFileTagsField.value;
            });

            
            _patchedAOTDLLFolderField= Root.Q<ObjectField>(nameof(_hybridBuilderSetting.PatchedAOTDLLFolder));
            _patchedAOTDLLFolderField.objectType = typeof(DefaultAsset);
            _patchedAOTDLLFolderField.SetValueWithoutNotify(_hybridBuilderSetting.PatchedAOTDLLFolder);
            _patchedAOTDLLFolderField.RegisterValueChangedCallback((evt) =>
            {
                var assetPath = AssetDatabase.GetAssetPath(evt.newValue);
                if (!Directory.Exists(assetPath))
                {
                    Debug.unityLogger.Log($"该文件不是文件夹类型 ===> {assetPath}");
                    _patchedAOTDLLFolderField.SetValueWithoutNotify(evt.previousValue);
                }
                _hybridBuilderSetting.PatchedAOTDLLFolder = evt.newValue as DefaultAsset;
            });

            
            _hotUpdateDLLFolderField= Root.Q<ObjectField>(nameof(_hybridBuilderSetting.HotUpdateDLLFolder));
            _hotUpdateDLLFolderField.objectType = typeof(DefaultAsset);
            _hotUpdateDLLFolderField.SetValueWithoutNotify(_hybridBuilderSetting.HotUpdateDLLFolder);
            _hotUpdateDLLFolderField.RegisterValueChangedCallback((evt) =>
            {
                var assetPath = AssetDatabase.GetAssetPath(evt.newValue);
                if (!Directory.Exists(assetPath))
                {
                    Debug.unityLogger.Log($"该文件不是文件夹类型 ===> {assetPath}");
                    _hotUpdateDLLFolderField.SetValueWithoutNotify(evt.previousValue);
                    return;
                }
                _hybridBuilderSetting.HotUpdateDLLFolder = evt.newValue as DefaultAsset;
            });

            
            
            // 清理构建缓存
            bool clearBuildCache = _hybridBuilderSetting.isClearBuildCache;
            _clearBuildCacheToggle = Root.Q<Toggle>("ClearBuildCache");
            _clearBuildCacheToggle.SetValueWithoutNotify(clearBuildCache);
            _clearBuildCacheToggle.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.isClearBuildCache = evt.newValue;
            });

            // 使用资源依赖数据库
            bool useAssetDependencyDB = _hybridBuilderSetting.isUseAssetDependDB;
                _useAssetDependencyDBToggle = Root.Q<Toggle>("UseAssetDependency");
            _useAssetDependencyDBToggle.SetValueWithoutNotify(useAssetDependencyDB);
            _useAssetDependencyDBToggle.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.isUseAssetDependDB = evt.newValue;
            });
            
            //热更新脚本混合构建选项
            var hybridBuildOption =Root.Q<EnumField>("HybridBuildOption");
            hybridBuildOption.Init(_hybridBuilderSetting.hybridBuildOption);
            hybridBuildOption.SetValueWithoutNotify(_hybridBuilderSetting.hybridBuildOption);
            hybridBuildOption.style.width = StyleWidth;
            hybridBuildOption.RegisterValueChangedCallback(evt =>
            {
                _hybridBuilderSetting.hybridBuildOption = (HybridBuildOption)evt.newValue;
                RefreshScriptCollectorGourpNameView();
            });

            // 对齐文本间距
            UIElementsTools.SetElementLabelMinWidth(_buildOutputField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_buildVersionField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_compressionField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_encryptionField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_outputNameStyleField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_copyBuildinFileOptionField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_copyBuildinFileTagsField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_clearBuildCacheToggle, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_useAssetDependencyDBToggle, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(hybridBuildOption, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(assetPakcageOption, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(scriptPackageOption, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_patchedAOTDLLFolderField, LabelMinWidth);
            UIElementsTools.SetElementLabelMinWidth(_hotUpdateDLLFolderField, LabelMinWidth);
            
            // 构建按钮
            var buildButton = Root.Q<Button>("Build");
            buildButton.clicked += BuildButton_clicked;

            return true;
        }
        

        private void OnVersionToggleChange(ChangeEvent<bool> evt)
        {
            _hybridBuilderSetting.isUseSelfIncrementingVersions = evt.newValue;
            if (_hybridBuilderSetting.isUseSelfIncrementingVersions)
            {
                _buildVersionField.SetValueWithoutNotify(_hybridBuilderSetting.GetApplicationBuildVersion(false));
            }
            else
            {
                _buildVersionField.SetValueWithoutNotify(GetDefaultPackageVersion());
            }
        }
        
        private void RefreshBuildinFileCopyOptionView()
        {
            var buildinFileCopyOption = _hybridBuilderSetting.assetBuildinFileCopyOption;
            bool tagsFiledVisible = buildinFileCopyOption == EBuildinFileCopyOption.ClearAndCopyByTags ||
                                    buildinFileCopyOption == EBuildinFileCopyOption.OnlyCopyByTags;
            _copyBuildinFileTagsField.visible = tagsFiledVisible;
        }

        private void RefreshScriptCollectorGourpNameView()
        {
            var buildOption = _hybridBuilderSetting.hybridBuildOption;
            bool nameFiledVisible = buildOption == HybridBuildOption.BuildAll ||
                                    buildOption == HybridBuildOption.BuildApplication ||
                                    buildOption == HybridBuildOption.BuildScript;
            
            _patchedAOTDLLFolderField.visible = nameFiledVisible;
            _hotUpdateDLLFolderField.visible = nameFiledVisible;
        }
        private void BuildButton_clicked()
        {
            if (EditorUtility.DisplayDialog("提示", $"开始以[{_hybridBuilderSetting.name}]配置->构建资源包！", "Yes", "No"))
            {
                EditorTools.ClearUnityConsole();
                EditorApplication.delayCall += ExecuteBuild;
            }
            else
            {
                Debug.LogWarning("[Build] 打包已经取消");
            }
        }

        /// <summary>
        /// 执行构建任务
        /// </summary>
        protected abstract void ExecuteBuild();

        /// <summary>
        /// 获取构建版本
        /// </summary>
        protected string GetPackageVersion()
        {
            return _buildVersionField.value;
        }

        /// <summary>
        /// 创建加密类实例
        /// </summary>
        protected IEncryptionServices CreateEncryptionInstance()
        {
            var encyptionClassName = _hybridBuilderSetting.assetEncyptionClassName;
            var encryptionClassTypes = EditorTools.GetAssignableTypes(typeof(IEncryptionServices));
            var classType = encryptionClassTypes.Find(x => x.FullName.Equals(encyptionClassName));
            if (classType != null)
                return (IEncryptionServices) Activator.CreateInstance(classType);
            else
                return null;
        }

        private string GetDefaultPackageVersion()
        {
            int totalMinutes = DateTime.Now.Hour * 60 + DateTime.Now.Minute;
            return DateTime.Now.ToString("yyyy-MM-dd") + "-" + totalMinutes;
        }


        private void BrowserFolder(string defaultPath,Action<string> callBack)
        {
            string selectFolder = EditorUtility.OpenFolderPanel("Select Output Path",defaultPath,string.Empty);
            if (!string.IsNullOrEmpty(selectFolder))
            {
                callBack?.Invoke(selectFolder);
            }
        }

        private List<string> GetBuildPackageNames()
        {
            List<string> result = new List<string>();
            foreach (var package in AssetBundleCollectorSettingData.Setting.Packages)
            {
                result.Add(package.PackageName);
            }

            return result;
        }
    }
}
#endif