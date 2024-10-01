﻿using Microsoft.Win32;
using System.ComponentModel;
using System.IO;
using System.Windows;
using System.Windows.Input;
using TRX_ConfigToolLib.Controls;
using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models;

public class MainWindowViewModel : BaseLanguageViewModel
{
    private readonly Configuration _configuration;

    public IEnumerable<CategoryViewModel> Categories { get; private set; }

    public MainWindowViewModel()
    {
        _configuration = new();

        List<CategoryViewModel> categories = new();
        foreach (Category category in _configuration.Categories.Where(c => c.Properties.Count > 0))
        {
            categories.Add(new(category));
            category.Properties
                .ForEach(p => p.PropertyChanged += EditorPropertyChanged);
        }

        Categories = categories;
        SelectedCategory = Categories.FirstOrDefault();
    }

    private void EditorPropertyChanged(object sender, PropertyChangedEventArgs e)
    {
        IsEditorDirty = _configuration.IsDataDirty();
        IsEditorDefault = _configuration.IsDataDefault();
    }

    public void Load()
    {
        Open(Path.GetFullPath(TRXConstants.Instance.ConfigPath));
    }

    private CategoryViewModel _selectedCategory;
    public CategoryViewModel SelectedCategory
    {
        get => _selectedCategory;
        set
        {
            _selectedCategory = value;
            NotifyPropertyChanged();
        }
    }

    private bool _isEditorDirty;
    public bool IsEditorDirty
    {
        get => _isEditorDirty;
        set
        {
            if (_isEditorDirty != value)
            {
                _isEditorDirty = value;
                NotifyPropertyChanged();
            }
        }
    }

    private bool _isEditorDefault;
    public bool IsEditorDefault
    {
        get => _isEditorDefault;
        set
        {
            if (_isEditorDefault != value)
            {
                _isEditorDefault = value;
                NotifyPropertyChanged();
            }
        }
    }

    private string _selectedFile;
    public string SelectedFile
    {
        get => _selectedFile;
        set
        {
            if (_selectedFile != value)
            {
                _selectedFile = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged(nameof(IsEditorActive));
            }
        }
    }

    public bool IsEditorActive
    {
        get => SelectedFile != null;
    }

    private RelayCommand _openCommand;
    public ICommand OpenCommand
    {
        get => _openCommand ??= new RelayCommand(Open);
    }

    private void Open()
    {
        if (!ConfirmEditorSaveState())
        {
            return;
        }

        OpenFileDialog dialog = new()
        {
            Filter = ViewText["file_dialog_filter"] + TRXConstants.Instance.ConfigFilterExtension
        };
        if (IsEditorActive)
        {
            dialog.InitialDirectory = Path.GetDirectoryName(SelectedFile);
        }
        if (dialog.ShowDialog() ?? false)
        {
            Open(dialog.FileName);
        }
    }

    private void Open(string filePath)
    {
        try
        {
            _configuration.Read(filePath);
            SelectedFile = filePath;
            IsEditorDirty = false;
            IsEditorDefault = _configuration.IsDataDefault();
        }
        catch (Exception e)
        {
            MessageBoxUtils.ShowError(e.ToString(), ViewText["window_title_main"]);
        }
    }

    private RelayCommand _reloadCommand;
    public ICommand ReloadCommand
    {
        get => _reloadCommand ??= new RelayCommand(Reload, CanReload);
    }

    private void Reload()
    {
        if (ConfirmEditorReloadState())
        {
            Open(SelectedFile);
        }
    }

    private bool CanReload()
    {
        return IsEditorActive;
    }

    private RelayCommand _saveCommand;
    public ICommand SaveCommand
    {
        get => _saveCommand ??= new RelayCommand(Save, CanSave);
    }

    private void Save()
    {
        Save(SelectedFile);
    }

    private void Save(string filePath)
    {
        try
        {
            _configuration.Write(filePath);
            SelectedFile = filePath;
            IsEditorDirty = false;
        }
        catch (Exception e)
        {
            MessageBoxUtils.ShowError(e.ToString(), ViewText["window_title_main"]);
        }
    }

    private bool CanSave()
    {
        return IsEditorDirty;
    }

    private RelayCommand _saveAsCommand;
    public ICommand SaveAsCommand
    {
        get => _saveAsCommand ??= new RelayCommand(SaveAs, CanSaveAs);
    }

    private void SaveAs()
    {
        SaveFileDialog dialog = new()
        {
            Filter = ViewText["file_dialog_filter"] + TRXConstants.Instance.ConfigFilterExtension,
            InitialDirectory = Path.GetDirectoryName(SelectedFile)
        };
        if (dialog.ShowDialog() ?? false)
        {
            Save(dialog.FileName);
        }
    }

    private bool CanSaveAs()
    {
        return IsEditorActive;
    }

    private RelayCommand _launchGameCommand;
    public ICommand LaunchGameCommand
    {
        get => _launchGameCommand ??= new RelayCommand(() => LaunchGame());
    }

    public static bool CanLaunchGold
    {
        get => TRXConstants.Instance.GoldSupported;
    }

    private RelayCommand _launchGoldCommand;
    public ICommand LaunchGoldCommand
    {
        get => _launchGoldCommand ??= new RelayCommand(() => LaunchGame(TRXConstants.Instance.GoldArgs));
    }

    private void LaunchGame(string arguments = null)
    {
        if (!ConfirmEditorSaveState())
        {
            return;
        }

        try
        {
            ProcessUtils.Start(Path.GetFullPath(TRXConstants.Instance.ExecutableName), arguments);
        }
        catch (Exception e)
        {
            MessageBoxUtils.ShowError(e.ToString(), ViewText["window_title_main"]);
        }
    }

    private RelayCommand<Window> _exitCommand;
    public ICommand ExitCommand
    {
        get => _exitCommand ??= new RelayCommand<Window>(Exit);
    }

    private void Exit(Window window)
    {
        if (ConfirmEditorSaveState())
        {
            IsEditorDirty = false;
            window.Close();
        }
    }

    public void Exit(CancelEventArgs e)
    {
        if (!ConfirmEditorSaveState())
        {
            e.Cancel = true;
        }
    }

    public bool ConfirmEditorSaveState()
    {
        if (IsEditorDirty)
        {
            switch (MessageBoxUtils.ShowYesNoCancel(ViewText["msgbox_unsaved_changes"], ViewText["window_title_main"]))
            {
                case MessageBoxResult.Yes:
                    Save();
                    break;
                case MessageBoxResult.Cancel:
                    return false;
            }
        }
        return true;
    }

    public bool ConfirmEditorReloadState()
    {
        return !IsEditorDirty
            || MessageBoxUtils.ShowYesNo(ViewText["msgbox_unsaved_changes_reload"], ViewText["window_title_main"]);
    }

    private RelayCommand _restoreDefaultsCommand;
    public ICommand RestoreDefaultsCommand
    {
        get => _restoreDefaultsCommand ??= new RelayCommand(RestoreDefaults, CanRestoreDefaults);
    }

    private void RestoreDefaults()
    {
        _configuration.RestoreDefaults();
    }

    private bool CanRestoreDefaults()
    {
        return IsEditorActive && !IsEditorDefault;
    }

    private RelayCommand _gitHubCommand;
    public ICommand GitHubCommand
    {
        get => _gitHubCommand ??= new RelayCommand(GoToGitHub);
    }

    private void GoToGitHub()
    {
        ProcessUtils.Start(TRXConstants.Instance.GitHubURL);
    }

    private RelayCommand _aboutCommand;
    public ICommand AboutCommand
    {
        get => _aboutCommand ??= new RelayCommand(ShowAboutDialog);
    }

    private void ShowAboutDialog()
    {
        new AboutWindow().ShowDialog();
    }
}
