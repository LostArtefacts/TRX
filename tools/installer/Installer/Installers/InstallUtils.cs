using System;
using System.IO;
using System.IO.Compression;
using System.Threading.Tasks;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using Installer.Utils;

namespace Installer.Installers;

public static class InstallUtils
{
    public static async Task CopyDirectoryTree(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        Func<string, bool>? filterCallback = null,
        Func<string, bool>? overwriteCallback = null
    )
    {
        try
        {
            progress.Report(new InstallProgress { Description = "Scanning directory" });
            var files = Directory.GetFiles(sourceDirectory, "*", SearchOption.AllDirectories);
            var currentProgress = 0;
            var maximumProgress = files.Length;
            foreach (var sourcePath in files)
            {
                if (filterCallback is not null && !filterCallback(sourcePath))
                {
                    continue;
                }
                var relPath = Path.GetRelativePath(sourceDirectory, sourcePath);
                var targetPath = Path.Combine(targetDirectory, relPath);
                var isSamePath = string.Equals(Path.GetFullPath(sourcePath), Path.GetFullPath(targetPath), StringComparison.OrdinalIgnoreCase);
                if (!File.Exists(targetPath) || (overwriteCallback is not null && overwriteCallback(sourcePath)) && !isSamePath)
                {
                    progress.Report(new InstallProgress
                    {
                        CurrentValue = currentProgress,
                        MaximumValue = maximumProgress,
                        Description = $"Copying {relPath}",
                    });
                    Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);
                    await Task.Run(() => File.Copy(sourcePath, targetPath, true));
                }
                else
                {
                    progress.Report(new InstallProgress
                    {
                        CurrentValue = currentProgress,
                        MaximumValue = maximumProgress,
                        Description = $"Copying {relPath} - skipped",
                    });
                }

                currentProgress++;
            }
        }
        catch (Exception e)
        {
            throw new ApplicationException($"Could not extract ZIP:\n{e.Message}");
        }
    }

    public static void CreateDesktopShortcut(string name, string targetPath, string[]? args = null)
    {
        var shortcutPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), $"{name}.lnk");
        ShortcutUtils.CreateShortcut(shortcutPath, targetPath, "Tomb Raider I: Community Edition", args);
    }

    public static async Task<byte[]> DownloadFile(string url, IProgress<InstallProgress> progress)
    {
        HttpProgressClient wc = new();
        progress.Report(new InstallProgress { Description = $"Initializing download of {url}" });
        wc.DownloadProgressChanged += (totalBytesToReceive, bytesReceived) =>
        {
            progress.Report(new InstallProgress
            {
                CurrentValue = (int)bytesReceived,
                MaximumValue = (int)totalBytesToReceive,
                Description = $"Downloading {url}",
            });
        };
        return await wc.DownloadDataTaskAsync(new Uri(url));
    }

    public static async Task DownloadZip(
        string url,
        string targetDirectory,
        IProgress<InstallProgress> progress
    )
    {
        var response = await DownloadFile(url, progress);
        using var stream = new MemoryStream(response);
        await ExtractZip(stream, targetDirectory, progress);
    }

    public static async Task ExtractZip(
        Stream stream,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        Func<string, bool>? filterCallback = null,
        bool overwrite = false
    )
    {
        try
        {
            using var zip = new ZipArchive(stream);
            progress.Report(new InstallProgress
            {
                Description = "Scanning embedded ZIP",
            });
            var currentProgress = 0;
            var maximumProgress = zip.Entries.Count;
            foreach (var entry in zip.Entries)
            {
                if (new Regex(@"[\\/]$").IsMatch(entry.FullName))
                {
                    continue;
                }
                if (filterCallback is not null && !filterCallback(entry.FullName))
                {
                    continue;
                }
                var targetPath = Path.Combine(
                        targetDirectory,
                        new Regex(@"[\\/]").Replace(entry.FullName, Path.DirectorySeparatorChar.ToString()));

                if (!File.Exists(targetPath) || overwrite)
                {
                    progress.Report(new InstallProgress
                    {
                        CurrentValue = currentProgress,
                        MaximumValue = maximumProgress,
                        Description = $"Extracting {entry.FullName}",
                    });

                    Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);
                    await Task.Run(() => entry.ExtractToFile(targetPath, true));
                }
                else
                {
                    progress.Report(new InstallProgress
                    {
                        CurrentValue = currentProgress,
                        MaximumValue = maximumProgress,
                        Description = $"Extracting {entry.FullName} - skipped",
                    });
                }

                currentProgress++;
            }
        }
        catch (Exception e)
        {
            throw new ApplicationException($"Could not extract ZIP:\n{e.Message}");
        }
    }

    public static IEnumerable<string> GetDesktopShortcutDirectories()
    {
        foreach (
            var shortcutPath in Directory.EnumerateFiles(
                Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "*.lnk"
            )
        )
        {
            string? lnkPath;
            try
            {
                lnkPath = ShortcutUtils.GetLnkTargetPath(shortcutPath);
            }
            catch (Exception)
            {
                continue;
            }
            if (lnkPath is not null)
            {
                var dirName = Path.GetDirectoryName(lnkPath);
                if (dirName is not null)
                {
                    yield return dirName;
                }
            }
        }
    }
}
