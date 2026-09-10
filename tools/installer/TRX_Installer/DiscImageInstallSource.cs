using DiscUtils.Iso9660;
using DiscUtils.Streams;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace TRX_Installer;

internal class DiscImageInstallSource : IInstallSource
{
    // Reports whether the directory holds a disc image this source can read.
    public static bool CanInstallFrom(string sourceDirectory)
    {
        return FindCueFile(sourceDirectory) is not null
            || FindRawImage(sourceDirectory) is not null;
    }

    public async Task InstallAsync(
        string sourceDirectory,
        string targetDirectory,
        string gameId,
        IInstallerProgress progress)
    {
        (string isoPath, bool isTemporary) = PrepareIso(
            sourceDirectory, Path.Combine(sourceDirectory, "game.iso"));

        try
        {
            using FileStream file = File.Open(isoPath, FileMode.Open, FileAccess.Read);
            using CDReader reader = new(file, true);
            List<string> filesToExtract = GetFilesToExtract(reader.Root).ToList();
            HashSet<string> availablePaths = filesToExtract
                .Select(InstallMappings.Normalize)
                .ToHashSet(StringComparer.OrdinalIgnoreCase);
            progress.SetSubDescription("Extracting files from disc image...");
            progress.SetInnerProgress(0, Math.Max(1, filesToExtract.Count));

            int done = 0;
            foreach (string path in filesToExtract)
            {
                string? relPath = InstallMappings.MapOriginalFile(gameId, path, availablePaths);
                if (relPath is null)
                {
                    done++;
                    progress.SetInnerProgress(done, Math.Max(1, filesToExtract.Count));
                    continue;
                }

                string targetPath = Path.Combine(targetDirectory, relPath);
                Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);
                using SparseStream sourceStream = reader.OpenFile(path, FileMode.Open, FileAccess.Read);
                byte[] buffer = new byte[sourceStream.Length];
                sourceStream.Read(buffer, 0, buffer.Length);
                await File.WriteAllBytesAsync(targetPath, buffer);
                done++;
                progress.SetInnerProgress(done, Math.Max(1, filesToExtract.Count));
                progress.AppendLog($"Extracting {relPath}");
            }
        }
        finally
        {
            if (isTemporary && File.Exists(isoPath))
            {
                File.Delete(isoPath);
            }
        }
    }

    // Layout of the sectors in a disc image. A plain ISO stores 2048 data
    // bytes per sector, while a raw image stores 2352 bytes per sector and
    // keeps the data at a fixed offset that depends on the sector mode.
    private enum DiscImageKind
    {
        Unknown,
        Iso,
        RawMode1,
        RawMode2,
    }

    private static readonly string[] ImageFileNames = ["GAME.GOG", "game.dat"];
    private static readonly string[] ImageFilePatterns = ["*.iso", "*.bin", "*.gog", "*.dat", "*.img"];

    // Returns the path to an image that CDReader can read, and whether that
    // image is a temporary file the caller must delete. A cue sheet gives the
    // position of the data track, and its first track goes to targetPath. A
    // raw image without a cue sheet goes to targetPath sector by sector. A
    // plain ISO needs no conversion and is read where it lies.
    private static (string path, bool isTemporary) PrepareIso(
        string sourceDirectory, string targetPath)
    {
        CueFile? cueFile = FindCueFile(sourceDirectory);
        if (cueFile is not null)
        {
            cueFile.TrackList.First().Write(targetPath, null);
            return (targetPath, true);
        }

        string? imagePath = FindRawImage(sourceDirectory);
        if (imagePath is null)
        {
            throw new ApplicationException(
                $"Could not find a disc image in {sourceDirectory}. "
                + "Expected a cue sheet with a matching image, an ISO image, "
                + "or a raw CD image.");
        }

        DiscImageKind kind = GetImageKind(imagePath);
        if (kind == DiscImageKind.Iso)
        {
            return (imagePath, false);
        }

        long length = new FileInfo(imagePath).Length;
        CueTrack track = new(
            imagePath,
            1,
            kind == DiscImageKind.RawMode2 ? "MODE2/2352" : "MODE1/2352",
            "00:00:00")
        {
            Stop = length,
            StopSector = length / CueTrack.SectorLength,
        };
        track.Write(targetPath, null);
        return (targetPath, true);
    }

    private static CueFile? FindCueFile(string sourceDirectory)
    {
        return EnumerateCandidates(sourceDirectory, ["*.cue"])
            .Select(CueFile.TryLoad)
            .FirstOrDefault(cueFile => cueFile is not null);
    }

    private static string? FindRawImage(string sourceDirectory)
    {
        return EnumerateCandidates(sourceDirectory, ImageFilePatterns)
            .FirstOrDefault(path => GetImageKind(path) != DiscImageKind.Unknown);
    }

    // Lists the files to probe, with the names the original releases use
    // first, and never the temporary image this source writes itself.
    private static IEnumerable<string> EnumerateCandidates(
        string sourceDirectory, string[] patterns)
    {
        if (!Directory.Exists(sourceDirectory))
        {
            yield break;
        }

        HashSet<string> seen = new(StringComparer.OrdinalIgnoreCase);
        IEnumerable<string> paths = ImageFileNames
            .Select(name => Path.Combine(sourceDirectory, name))
            .Concat(patterns.SelectMany(pattern =>
                Directory.EnumerateFiles(sourceDirectory, pattern)));

        foreach (string path in paths)
        {
            if (Path.GetFileName(path).Equals("game.iso", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }
            if (File.Exists(path) && seen.Add(Path.GetFullPath(path)))
            {
                yield return path;
            }
        }
    }

    private static DiscImageKind GetImageKind(string path)
    {
        try
        {
            using FileStream stream = File.OpenRead(path);
            if (HasVolumeDescriptor(stream, 16 * 2048 + 1))
            {
                return DiscImageKind.Iso;
            }
            if (HasVolumeDescriptor(stream, 16 * CueTrack.SectorLength + 16 + 1))
            {
                return DiscImageKind.RawMode1;
            }
            if (HasVolumeDescriptor(stream, 16 * CueTrack.SectorLength + 24 + 1))
            {
                return DiscImageKind.RawMode2;
            }
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
        }
        return DiscImageKind.Unknown;
    }

    // Reports whether the primary volume descriptor of an ISO 9660 file
    // system starts at the given offset.
    private static bool HasVolumeDescriptor(FileStream stream, long offset)
    {
        byte[] buffer = new byte[5];
        if (stream.Length < offset + buffer.Length)
        {
            return false;
        }
        stream.Seek(offset, SeekOrigin.Begin);
        return stream.Read(buffer, 0, buffer.Length) == buffer.Length
            && Encoding.ASCII.GetString(buffer) == "CD001";
    }

    private static IEnumerable<string> GetFilesToExtract(DiscUtils.DiscDirectoryInfo root)
    {
        Regex regex = new(@"^(data|fmv|music)[\\/].*$", RegexOptions.IgnoreCase);
        foreach (DiscUtils.DiscDirectoryInfo dir in root.GetDirectories())
        {
            foreach (string filePath in GetFilesToExtract(dir))
            {
                yield return filePath;
            }
        }
        foreach (DiscUtils.DiscFileInfo file in root.GetFiles())
        {
            if (regex.IsMatch(file.FullName))
            {
                yield return file.FullName;
            }
        }
    }
}
