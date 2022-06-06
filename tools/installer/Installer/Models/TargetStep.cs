using Installer.Utils;
using System.Windows.Input;

namespace Installer.Models;

public class TargetStep : BaseNotifyPropertyChanged, IStep
{
    public TargetStep(InstallSourceViewModel installSource)
    {
        InstallSettings = new InstallSettings(installSource.InstallSource)
        {
            SourceDirectory = installSource.SourceDirectory
        };
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
