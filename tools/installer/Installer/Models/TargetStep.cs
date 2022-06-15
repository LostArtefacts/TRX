using Installer.Utils;
using System.Windows.Input;

namespace Installer.Models;

public class TargetStep : BaseNotifyPropertyChanged, IStep
{
    public TargetStep(InstallSettings installSettings)
    {
        InstallSettings = installSettings;
        InstallSettings.PropertyChanged += (sender, e) =>
        {
            NotifyPropertyChanged(nameof(CanProceedToNextStep));
        };
    }

    public bool CanProceedToNextStep => InstallSettings.TargetDirectory != null;
    public bool CanProceedToPreviousStep => true;

    public ICommand ChooseLocationCommand
    {
        get
        {
            return _chooseLocationCommand ??= new RelayCommand(ChooseLocation);
        }
    }

    public InstallSettings InstallSettings { get; }
    public string SidebarImage => "pack://application:,,,/Tomb1Main_Installer;component/Resources/side2.jpg";
    private RelayCommand? _chooseLocationCommand;

    private void ChooseLocation()
    {
        var result = FileBrowser.Browse(InstallSettings.TargetDirectory);
        if (result is not null)
        {
            InstallSettings.TargetDirectory = result;
        }
    }
}
