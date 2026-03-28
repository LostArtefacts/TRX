using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Media;

namespace TRX_Installer;

public class InstallComponent : INotifyPropertyChanged
{
    public InstallComponent(
        string gameId,
        string title,
        string description,
        IEnumerable<InstallSourceOption>? sourceOptions = null,
        string? downloadUrl = null,
        IEnumerable<DownloadOption>? downloadOptions = null,
        IEnumerable<OptionalDownload>? optionalDownloads = null
    )
    {
        GameId = gameId;
        Title = title;
        Description = description;
        DownloadUrl = downloadUrl;
        DownloadOptions = new ObservableCollection<DownloadOption>(
            downloadOptions ?? Array.Empty<DownloadOption>());
        OptionalDownloads = new ObservableCollection<OptionalDownload>(
            optionalDownloads ?? Array.Empty<OptionalDownload>());
        SourceOptions = new ObservableCollection<InstallSourceOption>(
            sourceOptions ?? Array.Empty<InstallSourceOption>());
        SelectedDownloadOption = DownloadOptions.FirstOrDefault();
        AutoDetectSource();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Description { get; }
    public InstallSourceOption? DetectedSource
        => SourceDirectory is null
            ? null
            : SourceOptions.FirstOrDefault(option => option.IsGameFound(SourceDirectory));
    public ObservableCollection<DownloadOption> DownloadOptions { get; }
    public string? DownloadUrl { get; }
    public string GameId { get; }
    public bool HasDownload => DownloadUrl is not null || DownloadOptions.Count != 0;
    public bool HasOptionalDownloads => OptionalDownloads.Count != 0;
    public bool HasSource => SourceOptions.Count != 0;
    public bool HasDownloadOptions => DownloadOptions.Count > 1;
    public ObservableCollection<OptionalDownload> OptionalDownloads { get; }

    public bool Install
    {
        get => _install;
        set
        {
            if (value == _install)
            {
                return;
            }
            _install = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsReadyToInstall));
        }
    }

    public bool IsReadyToInstall
        => (HasSource && DetectedSource is not null) || (!HasSource && ResolvedDownloadUrl is not null);

    public string? ResolvedDownloadUrl
        => SelectedDownloadOption?.Url ?? DownloadUrl;

    public DownloadOption? SelectedDownloadOption
    {
        get => _selectedDownloadOption;
        set
        {
            if (value == _selectedDownloadOption)
            {
                return;
            }
            _selectedDownloadOption = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(ResolvedDownloadUrl));
            OnPropertyChanged(nameof(IsReadyToInstall));
        }
    }

    public string? SourceDirectory
    {
        get => _sourceDirectory;
        set
        {
            if (value == _sourceDirectory)
            {
                return;
            }
            _sourceDirectory = value;
            _isManualSourceSelected = false;
            OnPropertyChanged();
            OnPropertyChanged(nameof(DetectedSource));
            OnPropertyChanged(nameof(SourceStatus));
            OnPropertyChanged(nameof(SourceStatusBrush));
            OnPropertyChanged(nameof(IsReadyToInstall));
        }
    }

    public ObservableCollection<InstallSourceOption> SourceOptions { get; }

    public string SourceStatus
    {
        get
        {
            if (HasDownload && !HasSource)
            {
                return "(download)";
            }
            if (_isManualSourceSelected && DetectedSource is not null)
            {
                return "(manual source selected)";
            }
            if (DetectedSource is not null)
            {
                return $"(found via {DetectedSource.SourceName})";
            }
            return "(source not found)";
        }
    }

    public System.Windows.Media.Brush SourceStatusBrush
        => DetectedSource is not null || (HasDownload && !HasSource)
            ? System.Windows.Media.Brushes.ForestGreen
            : System.Windows.Media.Brushes.Firebrick;

    public string Title { get; }

    private bool _install;
    private bool _isManualSourceSelected;
    private DownloadOption? _selectedDownloadOption;
    private string? _sourceDirectory;

    public void SetManualSourceDirectory(string sourceDirectory)
    {
        if (sourceDirectory == _sourceDirectory)
        {
            return;
        }

        _sourceDirectory = sourceDirectory;
        _isManualSourceSelected = true;
        OnPropertyChanged(nameof(SourceDirectory));
        OnPropertyChanged(nameof(DetectedSource));
        OnPropertyChanged(nameof(SourceStatus));
        OnPropertyChanged(nameof(SourceStatusBrush));
        OnPropertyChanged(nameof(IsReadyToInstall));
    }

    private void AutoDetectSource()
    {
        foreach (InstallSourceOption option in SourceOptions)
        {
            foreach (string directory in option.DirectoriesToTry)
            {
                if (!option.IsGameFound(directory))
                {
                    continue;
                }
                SourceDirectory = directory;
                return;
            }
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
