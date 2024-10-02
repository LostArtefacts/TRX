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
            // NOTE: the order also decides which installation source will be selected by default
            new(new SteamInstallSource()),
            new(new GOGInstallSource()),
            new(new TombATIInstallSource()),
            new(new TR1XInstallSource()),
            new(new CDRomInstallSource()),
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

        foreach (var source in InstallationSources)
        {
            if (source.IsAvailable)
            {
                // TR1X comes last and always trumps any other installation source
                SelectedInstallationSource = source;
            }
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

    public string SidebarImage => "pack://application:,,,/TR1X_Installer;component/Resources/side1.jpg";
    private InstallSourceViewModel? selectedInstallationSource;
}
