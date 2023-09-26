using Installer.Utils;
using System;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Input;

namespace Installer.Models;

public class MainWindowViewModel : BaseNotifyPropertyChanged
{
    public MainWindowViewModel()
    {
        _sourceStep = new SourceStep();
        _currentStep = _sourceStep;
        _installSettings = new InstallSettings();
    }

    public ICommand CloseWindowCommand
    {
        get
        {
            return _closeWindowCommand ??= new RelayCommand<Window>(CloseWindow);
        }
    }

    public IStep CurrentStep
    {
        get { return _currentStep; }
        set
        {
            _currentStep = value;
            _goToPreviousStepCommand?.RaiseCanExecuteChanged();
            _goToNextStepCommand?.RaiseCanExecuteChanged();
            _currentStep.PropertyChanged += (sender, e) =>
            {
                _goToPreviousStepCommand?.RaiseCanExecuteChanged();
                _goToNextStepCommand?.RaiseCanExecuteChanged();
            };
            NotifyPropertyChanged();
            NotifyPropertyChanged(nameof(IsFinalStep));
        }
    }

    public ICommand GoToNextStepCommand
    {
        get
        {
            return _goToNextStepCommand ??= new RelayCommand(GoToNextStep, CanGoToNextStep);
        }
    }

    public ICommand GoToPreviousStepCommand
    {
        get
        {
            return _goToPreviousStepCommand ??= new RelayCommand(GoToPreviousStep, CanGoToPreviousStep);
        }
    }

    public bool IsFinalStep
    {
        get
        {
            return CurrentStep is FinishStep;
        }
    }

    public bool IsSidebarVisible
    {
        get
        {
            return WindowWidth >= 500;
        }
    }

    public int WindowWidth
    {
        get => _windowWidth;
        set
        {
            if (value != _windowWidth)
            {
                _windowWidth = value;
                NotifyPropertyChanged(nameof(IsSidebarVisible));
            }
        }
    }

    private const bool _autoFinishInstallStep = false;

    private RelayCommand<Window>? _closeWindowCommand;

    private IStep _currentStep;
    private FinishSettings? _finishSettings;
    private RelayCommand? _goToNextStepCommand;
    private RelayCommand? _goToPreviousStepCommand;
    private InstallSettings _installSettings;
    private IStep _sourceStep;
    private int _windowWidth;

    private bool CanGoToNextStep()
    {
        return CurrentStep.CanProceedToNextStep;
    }

    private bool CanGoToPreviousStep()
    {
        return CurrentStep.CanProceedToPreviousStep;
    }

    private void CloseWindow(Window? window)
    {
        if (_finishSettings is not null && _finishSettings.LaunchGame)
        {
            if (_installSettings.TargetDirectory is null)
            {
                throw new NullReferenceException();
            }
            Process.Start(Path.Combine(_installSettings.TargetDirectory, "TR1X.exe"));
        }
        if (_finishSettings is not null && _finishSettings.OpenGameDirectory)
        {
            if (_installSettings.TargetDirectory is null)
            {
                throw new NullReferenceException();
            }
            Process.Start("explorer.exe", _installSettings.TargetDirectory);
        }
        window?.Close();
    }

    private void GoToNextStep()
    {
        if (CurrentStep is SourceStep sourceStep)
        {
            var installSource = sourceStep.SelectedInstallationSource!.InstallSource;
            _installSettings.InstallSource = installSource;
            _installSettings.SourceDirectory = sourceStep.SelectedInstallationSource.SourceDirectory;
            CurrentStep = new InstallSettingsStep(_installSettings);
        }
        else if (CurrentStep is InstallSettingsStep targetStep)
        {
            var installStep = new InstallStep(targetStep.InstallSettings);
            installStep.RunInstall();
            installStep.PropertyChanged += (sender, e) =>
            {
                if (_autoFinishInstallStep && installStep.CanProceedToNextStep)
                {
                    _finishSettings = new FinishSettings();
                    CurrentStep = new FinishStep(_finishSettings);
                }
            };
            CurrentStep = installStep;
        }
        else if (CurrentStep is InstallStep)
        {
            _finishSettings = new FinishSettings();
            CurrentStep = new FinishStep(_finishSettings);
        }
    }

    private void GoToPreviousStep()
    {
        CurrentStep = _sourceStep;
    }
}
