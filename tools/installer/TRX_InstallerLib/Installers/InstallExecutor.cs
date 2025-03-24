using System.IO;
using TRX_InstallerLib.Models;
using TRX_InstallerLib.Utils;

namespace TRX_InstallerLib.Installers;

public class InstallExecutor
{
    private static readonly string _resourceBaseURL;

    static InstallExecutor()
    {
        _resourceBaseURL = $"https://lostartefacts.dev/aux/{TRXConstants.Instance.Game!.ToLower()}";
    }

    private readonly InstallSettings _settings;

    public InstallExecutor(InstallSettings settings)
    {
        _settings = settings;
    }

    public IInstallSource? InstallSource
    {
        get => _settings.InstallSource;
    }

    public async Task ExecuteInstall(IProgress<InstallProgress> progress)
    {
        if (_settings.SourceDirectory is null)
        {
            throw new NullReferenceException();
        }
        if (_settings.TargetDirectory is null)
        {
            throw new NullReferenceException();
        }

        await CopyOriginalGameFiles(_settings.SourceDirectory, _settings.TargetDirectory, progress);
        await CopyTRXFiles(_settings.TargetDirectory, progress);
        if (_settings.DownloadMusic)
        {
            await DownloadMusicFiles(_settings.TargetDirectory, progress);
        }

        if (_settings.DownloadExpansionPack)
        {
            await DownloadExpansionFiles(_settings.TargetDirectory, _settings.ExpansionPackType, progress);
        }
        if (_settings.CreateDesktopShortcut)
        {
            CreateDesktopShortcut(_settings.TargetDirectory);
        }

        progress.Report(new InstallProgress { Description = "Finished", Finished = true });
    }

    protected async Task CopyOriginalGameFiles(string sourceDirectory, string targetDirectory, IProgress<InstallProgress> progress)
    {
        if (_settings.InstallSource is null)
        {
            throw new NullReferenceException();
        }
        await _settings.InstallSource.CopyOriginalGameFiles(sourceDirectory, targetDirectory, progress, _settings.ImportSaves);
    }

    protected static async Task CopyTRXFiles(string targetDirectory, IProgress<InstallProgress> progress)
    {
        InstallUtils.StoreInstallationPath(targetDirectory);

        progress.Report(new InstallProgress
        {
            CurrentValue = 0,
            MaximumValue = 1,
            Description = "Opening embedded ZIP",
        });

        using var stream = AssemblyUtils.GetResourceStream("Resources.release.zip", false)
            ?? throw new ApplicationException($"Could not open embedded ZIP.");
        await InstallUtils.ExtractZip(stream, targetDirectory, progress, overwrite: true);
    }

    protected static void CreateDesktopShortcut(string targetDirectory)
    {
        InstallUtils.CreateDesktopShortcut("TR1X", Path.Combine(targetDirectory, "TR1X.exe"));
        if (File.Exists(Path.Combine(targetDirectory, "data", "cat.phd")))
        {
            InstallUtils.CreateDesktopShortcut("TR1X - UB", Path.Combine(targetDirectory, "TR1X.exe"), new[] { "-gold" });
        }
    }

    protected static async Task DownloadMusicFiles(string targetDirectory, IProgress<InstallProgress> progress)
    {
        await InstallUtils.DownloadZip($"{_resourceBaseURL}/music.zip", targetDirectory, progress);
    }

    protected static async Task DownloadExpansionFiles(string targetDirectory, ExpansionPackType type, IProgress<InstallProgress> progress)
    {
        await InstallUtils.DownloadZip(
            $"{_resourceBaseURL}/trub-{type.ToString().ToLower()}.zip",
            targetDirectory, progress);
    }
}
