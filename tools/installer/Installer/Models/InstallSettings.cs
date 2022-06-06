using Installer.Installers;

namespace Installer.Models;

public class InstallSettings : BaseNotifyPropertyChanged
{
    public InstallSettings(IInstallSource installSource)
    {
        InstallSource = installSource;
        DownloadMusic = installSource.SuggestDownloadingMusic;
        DownloadUnfinishedBusiness = installSource.SuggestDownloadingUnfinishedBusiness;
        TargetDirectory = installSource.SuggestedInstallationDirectory;
    }

    public bool CreateDesktopShortcut
    {
        get => _createDesktopShortcut;
        set
        {
            if (value != _createDesktopShortcut)
            {
                _createDesktopShortcut = value;
                NotifyPropertyChanged();
            }
        }
    }

    public bool DownloadMusic
    {
        get => _downloadMusic;
        set
        {
            if (value != _downloadMusic)
            {
                _downloadMusic = value;
                NotifyPropertyChanged();
            }
        }
    }

    public bool DownloadUnfinishedBusiness
    {
        get => _downloadUnfinishedBusiness;
        set
        {
            if (value != _downloadUnfinishedBusiness)
            {
                _downloadUnfinishedBusiness = value;
                NotifyPropertyChanged();
            }
        }
    }

    public IInstallSource InstallSource { get; }

    public bool OverwriteAllFiles
    {
        get => _overrideAllFiles; set
        {
            if (value != _overrideAllFiles)
            {
                _overrideAllFiles = value;
                NotifyPropertyChanged();
            }
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
            }
        }
    }

    public string? TargetDirectory
    {
        get => _targetDirectory;
        set
        {
            if (value != _targetDirectory)
            {
                _targetDirectory = value;
                NotifyPropertyChanged();
            }
        }
    }

    private bool _createDesktopShortcut = true;
    private bool _downloadMusic = true;
    private bool _downloadUnfinishedBusiness = true;
    private bool _overrideAllFiles = false;
    private string? _sourceDirectory;
    private string? _targetDirectory;
}
