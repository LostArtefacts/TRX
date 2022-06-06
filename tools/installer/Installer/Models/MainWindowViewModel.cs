using Installer.Utils;
using System.Windows.Input;

namespace Installer.Models;

public class MainWindowViewModel : BaseNotifyPropertyChanged
{
    public MainWindowViewModel()
    {
        _currentStep = new SourceStep();
    }

    public string CloseButtonLabel
    {
        get
        {
            return CurrentStep is FinishStep ? "Finish" : "Cancel";
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
            NotifyPropertyChanged(nameof(CloseButtonLabel));
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

    private IStep _currentStep;
    private RelayCommand? _goToNextStepCommand;
    private RelayCommand? _goToPreviousStepCommand;

    private bool CanGoToNextStep()
    {
        return CurrentStep.CanProceedToNextStep;
    }

    private bool CanGoToPreviousStep()
    {
        return CurrentStep.CanProceedToPreviousStep;
    }

    private void GoToNextStep()
    {
        if (CurrentStep is SourceStep sourceStep)
        {
            CurrentStep = new TargetStep(sourceStep.SelectedInstallationSource!);
        }
        else if (CurrentStep is TargetStep targetStep)
        {
            var installStep = new InstallStep(targetStep.InstallSettings);
            installStep.RunInstall();
            installStep.PropertyChanged += (sender, e) =>
            {
                if (installStep.CanProceedToNextStep)
                {
                    CurrentStep = new FinishStep();
                }
            };
            CurrentStep = installStep;
        }
        else if (CurrentStep is InstallStep)
        {
            CurrentStep = new FinishStep();
        }
    }

    private void GoToPreviousStep()
    {
        CurrentStep = new SourceStep();
    }
}
