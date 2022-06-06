using Installer.Installers;
using Installer.Utils;
using System.Windows.Input;

namespace Installer.Models;

public class InstallSourceViewModel : BaseNotifyPropertyChanged
{
    public InstallSourceViewModel(IInstallSource source)
    {
        InstallSource = source;

        foreach (var directory in source.DirectoriesToTry)
        {
            if (InstallSource.IsGameFound(directory))
            {
                SourceDirectory = directory;
                break;
            }
        }
    }

    public ICommand ChooseLocationCommand
    {
        get
        {
            return _chooseLocationCommand ??= new RelayCommand(ChooseLocation);
        }
    }

    public IInstallSource InstallSource { get; private set; }

    public bool IsAvailable
    {
        get
        {
            return SourceDirectory != null && InstallSource.IsGameFound(SourceDirectory);
        }
    }

    public string? SourceDirectory
    {
        get => _sourceDirectory;
        set
        {
            if (value != _sourceDirectory)
            {
                _sourceDirectory = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged(nameof(IsAvailable));
            }
        }
    }

    private RelayCommand? _chooseLocationCommand;
    private string? _sourceDirectory;

    private void ChooseLocation()
    {
        var result = FileBrowser.Browse(SourceDirectory);
        if (result is not null)
        {
            SourceDirectory = result;
        }
    }
}
