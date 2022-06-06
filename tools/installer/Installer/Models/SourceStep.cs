using Installer.Installers;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Installer.Models;

public class SourceStep : BaseNotifyPropertyChanged, IStep
{
    public SourceStep()
    {
        InstallationSources = new ObservableCollection<InstallSourceViewModel>
        {
            new InstallSourceViewModel(new SteamInstallSource()),
            new InstallSourceViewModel(new GOGInstallSource()),
            new InstallSourceViewModel(new TombATIInstallSource()),
            new InstallSourceViewModel(new Tomb1MainInstallSource()),
        };

        foreach (var installationSource in InstallationSources)
        {
            installationSource.PropertyChanged += (sender, e) =>
            {
                NotifyPropertyChanged(nameof(InstallationSources));
                if (installationSource == selectedInstallationSource)
                {
                    NotifyPropertyChanged(nameof(SelectedInstallationSource));
                }
            };
        }
    }

    public bool CanProceedToNextStep
    {
        get
        {
            return SelectedInstallationSource != null && SelectedInstallationSource.IsAvailable;
        }
    }

    public bool CanProceedToPreviousStep => false;

    public IEnumerable<InstallSourceViewModel> InstallationSources { get; private set; }

    public InstallSourceViewModel? SelectedInstallationSource
    {
        get => selectedInstallationSource;
        set
        {
            if (value != selectedInstallationSource)
            {
                selectedInstallationSource = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged(nameof(CanProceedToNextStep));
            }
        }
    }

    private InstallSourceViewModel? selectedInstallationSource;
}
