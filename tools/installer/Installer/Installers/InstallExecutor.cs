using Installer.Models;
using Microsoft.Win32;
using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;

namespace Installer.Installers;

public class InstallExecutor
{
    public InstallExecutor(InstallSettings settings)
    {
        _settings = settings;
    }

    public IInstallSource? InstallSource
    {
        get
        {
            return _settings.InstallSource;
        }
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
        await CopyTomb1MainFiles(_settings.TargetDirectory, progress);
        if (_settings.DownloadMusic)
        {
            await DownloadMusicFiles(_settings.TargetDirectory, progress);
        }

        if (_settings.DownloadUnfinishedBusiness)
        {
            await DownloadUnfinishedBusinessFiles(_settings.TargetDirectory, progress);
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
        await _settings.InstallSource.CopyOriginalGameFiles(sourceDirectory, targetDirectory, progress, _settings.ImportSaves, _settings.OverwriteAllFiles);
    }

    protected async Task CopyTomb1MainFiles(string targetDirectory, IProgress<InstallProgress> progress)
    {
        using var key = Registry.CurrentUser.CreateSubKey(@"Software\Tomb1Main");
        if (key is not null)
        {
            key.SetValue("InstallPath", targetDirectory);
        }

        progress.Report(new InstallProgress
        {
            CurrentValue = 0,
            MaximumValue = 1,
            Description = "Opening embedded ZIP",
        });

        var assembly = Assembly.GetExecutingAssembly();
        var resourceName = assembly.GetManifestResourceNames().Where(n => n.EndsWith("release.zip")).First();
        using var stream = assembly.GetManifestResourceStream(resourceName);
        if (stream is null)
        {
            throw new ApplicationException($"Could not open embedded ZIP.");
        }

        var alwaysOverwrite = new string[]
        {
            ".bin",
            ".exe",
            ".fsh",
            ".json5",
            ".vsh",
        };

        await InstallUtils.ExtractZip(
            stream,
            targetDirectory,
            progress,
            overwriteCallback:
                file =>
                    _settings.OverwriteAllFiles
                    || alwaysOverwrite.Any(otherExt => string.Equals(Path.GetExtension(file), otherExt, StringComparison.CurrentCultureIgnoreCase))
        );
    }

    protected void CreateDesktopShortcut(string targetDirectory)
    {
        InstallUtils.CreateDesktopShortcut("Tomb1Main", Path.Combine(targetDirectory, "Tomb1Main.exe"));
        if (File.Exists(Path.Combine(targetDirectory, "data", "cat.phd")))
        {
            InstallUtils.CreateDesktopShortcut("Tomb1Main - UB", Path.Combine(targetDirectory, "Tomb1Main.exe"), new[] { "-gold" });
        }
    }

    protected async Task DownloadMusicFiles(string targetDirectory, IProgress<InstallProgress> progress)
    {
        await InstallUtils.DownloadZip("https://tmp.sakuya.pl/tomb1main/music.zip", targetDirectory, progress);
    }

    protected async Task DownloadUnfinishedBusinessFiles(string targetDirectory, IProgress<InstallProgress> progress)
    {
        await InstallUtils.DownloadZip("https://tmp.sakuya.pl/tomb1main/unfinished_business.zip", targetDirectory, progress);
    }

    private InstallSettings _settings;
}
