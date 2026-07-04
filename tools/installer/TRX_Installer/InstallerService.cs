using System.Collections.ObjectModel;
using System.IO;
using System.IO.Compression;
using System.Net.Http;
using System.Text.RegularExpressions;

namespace TRX_Installer;

internal class InstallerService
{
    private static readonly HttpClient HttpClient = new()
    {
        // Music packs can take longer than the framework's default 100-second timeout.
        Timeout = TimeSpan.FromMinutes(30),
    };

    public async Task RunInstallAsync(
        ObservableCollection<InstallComponent> allComponents,
        string targetDirectory,
        IInstallerProgress progress)
    {
        List<InstallComponent> selectedComponents = allComponents.Where(c => c.Install).ToList();
        progress.SetOuterProgress(0, 2 + selectedComponents.Count);

        progress.SetDescription("Extracting TRX...");
        progress.SetSubDescription("Preparing bundled files...");
        await ExtractEmbeddedReleaseAsync(targetDirectory, progress);
        progress.SetOuterProgress(1, 2 + selectedComponents.Count);

        int outerDone = 1;
        foreach (InstallComponent component in selectedComponents)
        {
            progress.SetDescription($"Installing {component.Title}...");
            progress.SetSubDescription(string.Empty);
            progress.AppendLog($"Installing {component.Title}");

            if (component.HasDownload)
            {
                if (component.DownloadUrl is not null)
                {
                    await DownloadAndExtractZipAsync(component.DownloadUrl, targetDirectory, progress);
                }
                if (component.SelectedDownloadOption is not null)
                {
                    await DownloadAndExtractZipAsync(component.SelectedDownloadOption.Url, targetDirectory, progress);
                }
            }

            if (component.HasSource)
            {
                if (component.SourceDirectory is null || component.DetectedSource is null)
                {
                    throw new ApplicationException($"No valid source configured for {component.Title}.");
                }
                await component.DetectedSource.Source.InstallAsync(
                    component.SourceDirectory, targetDirectory, component.GameId, progress);

                foreach (OptionalDownload optional in component.OptionalDownloads.Where(d => d.IsEnabled))
                {
                    await DownloadAndExtractZipAsync(optional.Url, targetDirectory, progress);
                }
            }

            outerDone++;
            progress.SetOuterProgress(outerDone, 2 + selectedComponents.Count);
        }

        string gamesDir = Path.Combine(targetDirectory, "games");
        if (Directory.Exists(gamesDir))
        {
            progress.SetDescription("Refreshing installed game list...");
            progress.SetSubDescription("Removing unchecked packs...");
            progress.SetInnerProgress(0, 1);
            var checkedIds = new HashSet<string>(
                allComponents.Where(c => c.Install).Select(c => c.GameId),
                StringComparer.OrdinalIgnoreCase);
            foreach (string dir in Directory.GetDirectories(gamesDir))
            {
                string name = Path.GetFileName(dir);
                if (!checkedIds.Contains(name)
                    && !name.EndsWith("-level", StringComparison.OrdinalIgnoreCase))
                {
                    progress.AppendLog($"Removing {name}");
                    await InstallFileHelper.DeleteDirectory(dir, recursive: true);
                    if (Directory.Exists(dir))
                    {
                        progress.AppendLog($"Failed to completely remove {name}");
                    }
                }
            }
        }
        outerDone++;
        progress.SetOuterProgress(outerDone, 2 + selectedComponents.Count);

        InstallComponentFactory.StoreInstallPath(targetDirectory);
        progress.SetDescription("Installation complete.");
        progress.SetSubDescription("All steps finished.");
        progress.AppendLog("Finished.");
    }

    private static async Task DownloadAndExtractZipAsync(
        string url,
        string targetDirectory,
        IInstallerProgress progress)
    {
        string fileName = Path.GetFileName(url);
        progress.SetDescription($"Downloading {fileName}...");
        progress.SetSubDescription("Starting download...");
        progress.AppendLog($"Downloading {url}");
        using HttpResponseMessage response = await HttpClient.GetAsync(
            url, HttpCompletionOption.ResponseHeadersRead);
        response.EnsureSuccessStatusCode();

        long? totalBytes = response.Content.Headers.ContentLength;
        using Stream responseStream = await response.Content.ReadAsStreamAsync();
        using MemoryStream stream = new();
        await CopyToMemoryStreamAsync(responseStream, stream, totalBytes, progress);
        stream.Position = 0;
        await ExtractZipAsync(stream, targetDirectory, progress);
    }

    private static async Task CopyToMemoryStreamAsync(
        Stream source,
        MemoryStream target,
        long? totalBytes,
        IInstallerProgress progress)
    {
        byte[] buffer = new byte[81920];
        long downloadedBytes = 0;

        if (totalBytes.HasValue)
        {
            progress.SetInnerProgress(0, 100);
        }
        else
        {
            progress.SetInnerProgress(0, 1);
        }

        while (true)
        {
            int bytesRead = await source.ReadAsync(buffer);
            if (bytesRead == 0)
            {
                break;
            }

            await target.WriteAsync(buffer.AsMemory(0, bytesRead));
            downloadedBytes += bytesRead;

            if (totalBytes.HasValue && totalBytes.Value > 0)
            {
                int percent = (int)(downloadedBytes * 100 / totalBytes.Value);
                progress.SetSubDescription(
                    $"Downloaded {FormatByteSize(downloadedBytes)} / {FormatByteSize(totalBytes.Value)} ({percent}%)");
                progress.SetInnerProgress(percent, 100);
            }
            else
            {
                progress.SetSubDescription($"Downloaded {FormatByteSize(downloadedBytes)}");
                progress.SetInnerProgress(0, 1);
            }
        }
    }

    private static string FormatByteSize(long byteCount)
    {
        string[] units = ["B", "KB", "MB", "GB"];
        double size = byteCount;
        int unitIndex = 0;

        while (size >= 1024 && unitIndex < units.Length - 1)
        {
            size /= 1024;
            unitIndex++;
        }

        return $"{size:0.0} {units[unitIndex]}";
    }

    private static async Task ExtractEmbeddedReleaseAsync(string targetDirectory, IInstallerProgress progress)
    {
        progress.SetSubDescription("Extracting bundled files...");
        progress.AppendLog("Opening embedded ZIP");
        using Stream stream = typeof(InstallerService).Assembly
            .GetManifestResourceStream("TRX_Installer.Resources.release.zip")
            ?? throw new ApplicationException("Could not open embedded ZIP.");
        await ExtractZipAsync(stream, targetDirectory, progress);
    }

    private static async Task ExtractZipAsync(
        Stream stream,
        string targetDirectory,
        IInstallerProgress progress)
    {
        using ZipArchive zip = new(stream);
        List<ZipArchiveEntry> entries = zip.Entries
            .Where(entry => !Regex.IsMatch(entry.FullName, @"[\\/]$"))
            .ToList();
        progress.SetInnerProgress(0, Math.Max(1, entries.Count));

        int done = 0;
        foreach (ZipArchiveEntry entry in entries)
        {
            string relPath = entry.FullName
                .Replace('/', Path.DirectorySeparatorChar)
                .Replace('\\', Path.DirectorySeparatorChar);
            string targetPath = Path.Combine(targetDirectory, relPath);
            Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);
            try
            {
                await Task.Run(() => entry.ExtractToFile(targetPath, true));
                progress.AppendLog($"Extracting {relPath}");
            }
            catch (IOException ex)
            {
                progress.AppendLog($"Skipping {relPath}: {ex.Message}");
            }
            done++;
            progress.SetInnerProgress(done, Math.Max(1, entries.Count));
        }
    }
}
