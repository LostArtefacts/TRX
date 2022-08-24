using DiscUtils.Iso9660;
using DiscUtils.Streams;
using Installer.Utils;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Installer.Installers;

public class GOGInstallSource : BaseInstallSource
{
    public override IEnumerable<string> DirectoriesToTry
    {
        get
        {
            yield return @"C:\Program Files (x86)\GOG Galaxy\Games\Tomb Raider 1";

            using var key = Registry.ClassesRoot.OpenSubKey(@"goggalaxy\shell\open\command");
            if (key is not null)
            {
                var value = key.GetValue("")?.ToString();
                if (value is not null && new Regex(@"""(?<path>[^""]+)""").Match(value) is { Success: true } match)
                {
                    yield return Path.Combine(Path.GetDirectoryName(match.Groups["path"].Value)!, @"Games\Tomb Raider 1");
                }
            }
        }
    }

    public override bool IsImportingSavesSupported => false;
    public override string SourceName => "GOG";

    public override Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool importSaves,
        bool overwrite
    )
    {
        var cuePath = Path.Combine(sourceDirectory, "game.dat");
        var isoPath = Path.Combine(sourceDirectory, "game.iso");
        CueFile cueFile;
        try
        {
            cueFile = new CueFile(cuePath);
        }
        catch (Exception e)
        {
            throw new ApplicationException($"Could not read CUE {cuePath}:\n{e.Message}");
        }

        try
        {
            var firstTrack = cueFile.TrackList.First();
            firstTrack.Write(isoPath, progress);
        }
        catch (Exception e)
        {
            throw new ApplicationException($"Could not convert BIN to ISO: {e.Message}");
        }

        try
        {
            using FileStream file = File.Open(isoPath, FileMode.Open, FileAccess.Read);
            using CDReader reader = new(file, true);
            int currentProgress = 0;

            progress.Report(new InstallProgress
            {
                MaximumValue = 1,
                CurrentValue = 0,
                Description = "Scanning the source directory",
            });
            var filesToExtract = GetFilesToExtract(reader.Root);
            progress.Report(new InstallProgress
            {
                MaximumValue = filesToExtract.Count(),
                CurrentValue = 0,
                Description = "Preparing to extract the ISO",
            });
            foreach (var path in filesToExtract)
            {
                var targetPath = Path.Combine(targetDirectory, path);
                if (!File.Exists(targetPath) || overwrite)
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);

                    using SparseStream sourceStream = reader.OpenFile(path, FileMode.Open, FileAccess.Read);
                    var readAllByte = new Byte[sourceStream.Length];
                    sourceStream.Read(readAllByte, 0, readAllByte.Length);

                    using FileStream targetStream = new FileStream(targetPath, FileMode.Create);
                    targetStream.Position = 0;
                    targetStream.Write(readAllByte, 0, readAllByte.Length);
                }

                progress.Report(new InstallProgress
                {
                    MaximumValue = filesToExtract.Count(),
                    CurrentValue = ++currentProgress,
                    Description = $"Extracting {path}",
                });
            }
        }
        catch (Exception e)
        {
            throw new ApplicationException($"Could not open converted ISO: {e.Message}");
        }

        File.Delete(isoPath);

        return Task.CompletedTask;
    }

    public override bool IsDownloadingMusicNeeded(string sourceDirectory)
    {
        return true;
    }

    public override bool IsDownloadingUnfinishedBusinessNeeded(string sourceDirectory)
    {
        return true;
    }

    public override bool IsGameFound(string sourceDirectory)
    {
        return File.Exists(Path.Combine(sourceDirectory, "GAME.GOG"));
    }

    private IEnumerable<string> GetFilesToExtract(DiscUtils.DiscDirectoryInfo root)
    {
        var regex = new Regex(@"^(data|fmv)[\\/].*$", RegexOptions.IgnoreCase);
        foreach (var dir in root.GetDirectories())
        {
            foreach (var filePath in GetFilesToExtract(dir))
            {
                yield return filePath;
            }
        }
        foreach (var file in root.GetFiles())
        {
            string filePath = file.FullName;
            if (regex.IsMatch(filePath))
            {
                yield return filePath;
            }
        }
    }
}
