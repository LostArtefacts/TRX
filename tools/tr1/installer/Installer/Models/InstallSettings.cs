using Installer.Installers;

namespace Installer.Models;

public class InstallSettings : BaseNotifyPropertyChanged
{
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

    public UBPackType UnfinishedBusinessType
    {
        get => _unfinishedBusinessType;
        set
        {
            if (value != _unfinishedBusinessType)
            {
                _unfinishedBusinessType = value;
                NotifyPropertyChanged();
            }
        }
    }

    public bool ImportSaves
    {
        get => _importSaves;
        set
        {
            if (value != _importSaves)
            {
                _importSaves = value;
                NotifyPropertyChanged();
            }
        }
    }

    public IInstallSource? InstallSource
    {
        get => _installSource;
        set
        {
            if (value != _installSource)
            {
                _installSource = value;
                DownloadMusic = SourceDirectory is not null && (_installSource?.IsDownloadingMusicNeeded(SourceDirectory) ?? false);
                DownloadUnfinishedBusiness = SourceDirectory is not null && (_installSource?.IsDownloadingUnfinishedBusinessNeeded(SourceDirectory) ?? false);
                ImportSaves = _installSource?.IsImportingSavesSupported ?? false;
                TargetDirectory = _installSource?.SuggestedInstallationDirectory;
                NotifyPropertyChanged();
            }
        }
    }

    public bool IsDownloadingMusicNeeded
    {
        get
        {
            return SourceDirectory is not null && (InstallSource?.IsDownloadingMusicNeeded(SourceDirectory) ?? false);
        }
    }

    public bool IsDownloadingUnfinishedBusinessNeeded
    {
        get
        {
            return SourceDirectory is not null && (InstallSource?.IsDownloadingUnfinishedBusinessNeeded(SourceDirectory) ?? false);
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
    private bool _downloadMusic;
    private bool _downloadUnfinishedBusiness;
    private UBPackType _unfinishedBusinessType;
    private bool _importSaves;
    private IInstallSource? _installSource;
    private string? _sourceDirectory;
    private string? _targetDirectory;
}
