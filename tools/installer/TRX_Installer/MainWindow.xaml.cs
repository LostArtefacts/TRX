using Microsoft.Win32;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Forms;

namespace TRX_Installer;

public enum InstallerStep
{
    Setup,
    Installing,
    Finish,
}

public partial class MainWindow : Window, INotifyPropertyChanged, IInstallerProgress
{
    private readonly InstallerService _installerService = new();

    public MainWindow()
    {
        InitializeComponent();
        DataContext = this;
        Components = new ObservableCollection<InstallComponent>(InstallComponentFactory.CreateComponents());
        foreach (InstallComponent component in Components)
        {
            component.PropertyChanged += (_, _) => OnPropertyChanged(nameof(CanGoNext));
        }
        TargetDirectory = InstallComponentFactory.GetStoredInstallPath()
            ?? Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),
                "TRX");
        RefreshComponentInstallStates();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    // ── Components ─────────────────────────────────────────────────────────────

    public ObservableCollection<InstallComponent> Components { get; }

    // ── Step navigation ─────────────────────────────────────────────────────────

    public InstallerStep CurrentStep
    {
        get => _currentStep;
        private set
        {
            if (value == _currentStep)
            {
                return;
            }
            _currentStep = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsSetupStep));
            OnPropertyChanged(nameof(IsInstallingStep));
            OnPropertyChanged(nameof(IsFinishStep));
            OnPropertyChanged(nameof(SidebarImage));
            OnPropertyChanged(nameof(CanGoNext));
            OnPropertyChanged(nameof(NextButtonText));
            OnPropertyChanged(nameof(CancelButtonText));
        }
    }

    public bool IsSetupStep => CurrentStep == InstallerStep.Setup;
    public bool IsInstallingStep => CurrentStep == InstallerStep.Installing;
    public bool IsFinishStep => CurrentStep == InstallerStep.Finish;

    public string SidebarImage => CurrentStep switch
    {
        InstallerStep.Installing => "/TRX_Installer;component/Resources/side2.jpg",
        InstallerStep.Finish => "/TRX_Installer;component/Resources/side3.jpg",
        _ => "/TRX_Installer;component/Resources/side1.jpg",
    };

    public bool IsSidebarVisible => WindowWidth >= 500;

    public int WindowWidth
    {
        get => _windowWidth;
        set
        {
            if (value == _windowWidth)
            {
                return;
            }
            _windowWidth = value;
            OnPropertyChanged(nameof(IsSidebarVisible));
        }
    }

    public bool CanGoNext => CurrentStep switch
    {
        InstallerStep.Setup =>
            !string.IsNullOrWhiteSpace(TargetDirectory)
            && Components.Any(c => c.Install)
            && Components.Where(c => c.Install).All(c => c.IsReadyToInstall),
        InstallerStep.Installing => _installFinished,
        _ => false,
    };

    public string NextButtonText => CurrentStep switch
    {
        InstallerStep.Setup => "_Install",
        InstallerStep.Installing when InstallFailed => "_Retry",
        _ => "_Next >",
    };

    public string CancelButtonText => IsFinishStep ? "_Close" : "_Cancel";

    // ── Finish step ─────────────────────────────────────────────────────────────

    public bool OpenFolder
    {
        get => _openFolder;
        set
        {
            if (value == _openFolder)
            {
                return;
            }
            _openFolder = value;
            OnPropertyChanged();
        }
    }

    public bool CreateShortcut
    {
        get => _createShortcut;
        set
        {
            if (value == _createShortcut)
            {
                return;
            }
            _createShortcut = value;
            OnPropertyChanged();
        }
    }

    // ── Install progress ────────────────────────────────────────────────────────

    public string InstallDescription
    {
        get => _installDescription;
        set
        {
            if (value == _installDescription)
            {
                return;
            }
            _installDescription = value;
            OnPropertyChanged();
        }
    }

    public string InstallSubDescription
    {
        get => _installSubDescription;
        set
        {
            if (value == _installSubDescription)
            {
                return;
            }
            _installSubDescription = value;
            OnPropertyChanged();
        }
    }

    // What went wrong, in the words of the person who has to fix it. The log
    // below it keeps the framework's own account.
    public string InstallErrorMessage
    {
        get => _installErrorMessage;
        set
        {
            if (value == _installErrorMessage)
            {
                return;
            }
            _installErrorMessage = value;
            OnPropertyChanged();
        }
    }

    public bool InstallFailed
    {
        get => _installFailed;
        set
        {
            if (value == _installFailed)
            {
                return;
            }
            _installFailed = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(NextButtonText));
        }
    }

    public int OuterCurrentProgress
    {
        get => _outerCurrentProgress;
        set
        {
            if (value == _outerCurrentProgress)
            {
                return;
            }
            _outerCurrentProgress = value;
            OnPropertyChanged();
        }
    }

    public int OuterMaximumProgress
    {
        get => _outerMaximumProgress;
        set
        {
            if (value == _outerMaximumProgress)
            {
                return;
            }
            _outerMaximumProgress = value;
            OnPropertyChanged();
        }
    }

    public int InnerCurrentProgress
    {
        get => _innerCurrentProgress;
        set
        {
            if (value == _innerCurrentProgress)
            {
                return;
            }
            _innerCurrentProgress = value;
            OnPropertyChanged();
        }
    }

    public int InnerMaximumProgress
    {
        get => _innerMaximumProgress;
        set
        {
            if (value == _innerMaximumProgress)
            {
                return;
            }
            _innerMaximumProgress = value;
            OnPropertyChanged();
        }
    }

    public string LogText
    {
        get => _logText;
        set
        {
            if (value == _logText)
            {
                return;
            }
            _logText = value;
            OnPropertyChanged();
        }
    }

    public string? TargetDirectory
    {
        get => _targetDirectory;
        set
        {
            if (value == _targetDirectory)
            {
                return;
            }
            _targetDirectory = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(CanGoNext));
            RefreshComponentInstallStates();
        }
    }

    // ── IInstallerProgress ──────────────────────────────────────────────────────

    void IInstallerProgress.SetDescription(string description)
    {
        InstallDescription = description;
    }

    void IInstallerProgress.SetSubDescription(string subDescription)
    {
        InstallSubDescription = subDescription;
    }

    void IInstallerProgress.SetOuterProgress(int current, int maximum)
    {
        OuterMaximumProgress = maximum;
        OuterCurrentProgress = current;
    }

    void IInstallerProgress.SetInnerProgress(int current, int maximum)
    {
        InnerMaximumProgress = maximum;
        InnerCurrentProgress = current;
    }

    void IInstallerProgress.AppendLog(string message)
    {
        LogText = string.IsNullOrEmpty(LogText)
            ? message
            : $"{LogText}{Environment.NewLine}{message}";
        _logTextBox.ScrollToEnd();
    }

    // ── Backing fields ──────────────────────────────────────────────────────────

    private InstallerStep _currentStep = InstallerStep.Setup;
    private bool _installFinished;
    private bool _openFolder = true;
    private bool _createShortcut;
    private string _installDescription = string.Empty;
    private string _installSubDescription = string.Empty;
    private string _installErrorMessage = string.Empty;
    private bool _installFailed;
    private int _outerCurrentProgress;
    private int _outerMaximumProgress = 1;
    private int _innerCurrentProgress;
    private int _innerMaximumProgress = 1;
    private string _logText = string.Empty;
    private string? _targetDirectory;
    private int _windowWidth;

    // ── Navigation button handlers ──────────────────────────────────────────────

    private async void Next_Click(object sender, RoutedEventArgs e)
    {
        if (CurrentStep == InstallerStep.Setup || InstallFailed)
        {
            await RunInstallAsync();
        }
        else if (CurrentStep == InstallerStep.Installing)
        {
            CurrentStep = InstallerStep.Finish;
        }
    }

    private void Cancel_Click(object sender, RoutedEventArgs e)
    {
        if (CurrentStep == InstallerStep.Finish)
        {
            ExecuteFinishActions();
        }
        Close();
    }

    private void ExecuteFinishActions()
    {
        if (TargetDirectory is null)
        {
            return;
        }

        if (CreateShortcut)
        {
            TryCreateDesktopShortcut(Path.Combine(TargetDirectory, "TRX.exe"));
        }

        if (OpenFolder)
        {
            System.Diagnostics.Process.Start("explorer.exe", TargetDirectory);
        }
    }

    private static void TryCreateDesktopShortcut(string targetExePath)
    {
        try
        {
            string shortcutPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),
                "TRX.lnk");
            Type? shellType = Type.GetTypeFromProgID("WScript.Shell");
            if (shellType is null)
            {
                return;
            }
            dynamic shell = Activator.CreateInstance(shellType)!;
            dynamic shortcut = shell.CreateShortcut(shortcutPath);
            shortcut.TargetPath = targetExePath;
            shortcut.WorkingDirectory = Path.GetDirectoryName(targetExePath);
            shortcut.Save();
        }
        catch
        {
            // Shortcut creation is best-effort.
        }
    }

    // ── Source/target folder choosers ───────────────────────────────────────────

    private void ChooseSource_Click(object sender, RoutedEventArgs e)
    {
        if (sender is not System.Windows.Controls.Button button
            || button.Tag is not InstallComponent component)
        {
            return;
        }
        string? result = BrowseFolder(component.SourceDirectory);
        if (result is not null)
        {
            component.SetManualSourceDirectory(result);
        }
    }

    private void ChooseTarget_Click(object sender, RoutedEventArgs e)
    {
        string? result = BrowseFolder(TargetDirectory);
        if (result is not null)
        {
            TargetDirectory = result;
        }
    }

    private void RefreshComponentInstallStates()
    {
        if (TargetDirectory is null)
        {
            return;
        }
        foreach (InstallComponent component in Components)
        {
            string gameDir = Path.Combine(TargetDirectory, "games", component.GameId);
            if (Directory.Exists(gameDir))
            {
                component.Install = true;
            }
        }
    }

    private static string? BrowseFolder(string? initialDirectory)
    {
        using FolderBrowserDialog dialog = new()
        {
            Description = "Choose directory",
            SelectedPath = initialDirectory ?? string.Empty,
            ShowNewFolderButton = true,
        };
        return dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK
            ? dialog.SelectedPath
            : null;
    }

    // ── Install orchestration ───────────────────────────────────────────────────

    private async Task RunInstallAsync()
    {
        if (TargetDirectory is null)
        {
            return;
        }

        CurrentStep = InstallerStep.Installing;
        _installFinished = false;
        InstallFailed = false;
        InstallErrorMessage = string.Empty;
        LogText = string.Empty;
        InstallSubDescription = string.Empty;
        OuterCurrentProgress = 0;
        OuterMaximumProgress = 1;
        InnerCurrentProgress = 0;
        InnerMaximumProgress = 1;

        try
        {
            await _installerService.RunInstallAsync(Components, TargetDirectory, this);
        }
        catch (Exception ex)
        {
            InstallFailed = true;
            InstallDescription = "Installation failed.";
            InstallSubDescription = string.Empty;
            InstallErrorMessage = ex is DownloadFailedException
                ? ex.Message
                : $"{ex.Message}{Environment.NewLine}{Environment.NewLine}"
                    + "The log below has the details.";
            ((IInstallerProgress)this).AppendLog(ex.ToString());
        }
        finally
        {
            _installFinished = true;
            OnPropertyChanged(nameof(CanGoNext));
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
