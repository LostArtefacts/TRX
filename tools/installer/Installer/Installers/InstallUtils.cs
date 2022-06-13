using System;
using System.IO;
using System.IO.Compression;
using System.Threading.Tasks;
using System.Net;
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
    ) {
        try
        {
            progress.Report(new InstallProgress { Description = "Scanning directory" });
            var files = Directory.GetFiles(sourceDirectory, "*", SearchOption.AllDirectories);
            var currentProgress = 0;
            var maximumProgress = files.Length;
            foreach (var file in files)
            {
                if (filterCallback is not null && !filterCallback(file))
                {
                    continue;
                }
                var relPath = Path.GetRelativePath(sourceDirectory, file);
                var targetPath = Path.Combine(targetDirectory, relPath);
                if (!File.Exists(targetPath) || (overwriteCallback is not null && overwriteCallback(file)))
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);
                    await Task.Run(() => File.Copy(file, targetPath, true));
                }

                progress.Report(new InstallProgress
                {
                    CurrentValue = ++currentProgress,
                    MaximumValue = maximumProgress,
                    Description = $"Copying {relPath}",
                });
            }
        }
        catch (Exception e)
        {
            throw new ApplicationException($"Could not extract ZIP:\n{e.Message}");
        }
    }

    public static void CreateDesktopShortcut(string path)
    {
        var shortcutPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "Tomb1Main.lnk");
        ShortcutUtils.CreateShortcut(path, shortcutPath, "Tomb Raider I: Community Edition");
    }

    public static async Task<byte[]> DownloadFile(string url, IProgress<InstallProgress> progress)
    {
        using var wc = new WebClient();
        progress.Report(new InstallProgress { Description = "Initializing download" });
        wc.DownloadProgressChanged += (sender, e) =>
        {
            progress.Report(new InstallProgress
            {
                CurrentValue = (int)e.BytesReceived,
                MaximumValue = (int)e.TotalBytesToReceive,
                Description = $"Downloading {url}",
            });
        };
        return await wc.DownloadDataTaskAsync(new Uri(url));
    }

    public static async Task DownloadZip(
        string url,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        Func<string, bool>? overwriteCallback = null
    ) {
        var response = await DownloadFile(url, progress);
        using var stream = new MemoryStream(response);
        await ExtractZip(stream, targetDirectory, progress, overwriteCallback);
    }

    public static async Task ExtractZip(
        Stream stream,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        Func<string, bool>? filterCallback = null,
        Func<string, bool>? overwriteCallback = null
    ) {
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

                if (!File.Exists(targetPath) || (overwriteCallback is not null && overwriteCallback(entry.FullName)))
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);
                    await Task.Run(() => entry.ExtractToFile(targetPath, true));
                }

                progress.Report(new InstallProgress
                {
                    CurrentValue = ++currentProgress,
                    MaximumValue = maximumProgress,
                    Description = $"Extracting {entry.FullName}",
                });
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
        ) {
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
                yield return Path.GetDirectoryName(lnkPath)!;
            }
        }
    }
}
