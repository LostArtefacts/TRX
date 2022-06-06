using Installer.Installers;
using System;
using System.Threading.Tasks;
using System.Windows;

namespace Installer.Models;

public class InstallStep : BaseNotifyPropertyChanged, IStep
{
    public InstallStep(InstallSettings installSettings)
    {
        InstallSettings = installSettings;
    }

    public bool CanProceedToNextStep
    {
        get => _canProceedToNextStep;
        set
        {
            if (value != _canProceedToNextStep)
            {
                _canProceedToNextStep = value;
                NotifyPropertyChanged();
            }
        }
    }

    public bool CanProceedToPreviousStep => false;

    public int CurrentProgress
    {
        get { return _currentProgress; }
        set
        {
            if (value != _currentProgress)
            {
                _currentProgress = value;
                NotifyPropertyChanged();
            }
        }
    }

    public string? Description
    {
        get => _description;
        set
        {
            if (value != _description)
            {
                _description = value;
                NotifyPropertyChanged();
            }
        }
    }

    public InstallSettings InstallSettings { get; }

    public int MaximumProgress
    {
        get { return _maximumProgress; }
        set
        {
            if (value != _maximumProgress)
            {
                _maximumProgress = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged(nameof(CanProceedToNextStep));
            }
        }
    }

    public void RunInstall()
    {
        var progress = new Progress<InstallProgress>();
        progress.ProgressChanged += (sender, progress) =>
        {
            CurrentProgress = progress.CurrentValue;
            MaximumProgress = progress.MaximumValue;
            Description = progress.Description;
            if (progress.Finished)
            {
                CanProceedToNextStep = true;
            }
        };

        Task.Run(async () =>
        {
            try
            {
                var executor = new InstallExecutor(InstallSettings);
                await executor.ExecuteInstall(progress);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        });
    }

    private bool _canProceedToNextStep;
    private int _currentProgress = 0;
    private string? _description;
    private int _maximumProgress = 1;
}
