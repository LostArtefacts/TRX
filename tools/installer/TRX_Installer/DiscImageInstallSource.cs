using DiscUtils.Iso9660;
using DiscUtils.Streams;
using System.IO;
using System.Text.RegularExpressions;

namespace TRX_Installer;

internal class DiscImageInstallSource : IInstallSource
{
    public async Task InstallAsync(
        string sourceDirectory,
        string targetDirectory,
        string gameId,
        IInstallerProgress progress)
    {
        string cuePath = Path.Combine(sourceDirectory, "game.dat");
        string isoPath = Path.Combine(sourceDirectory, "game.iso");
        CueFile cueFile = new(cuePath);
        CueTrack firstTrack = cueFile.TrackList.First();
        firstTrack.Write(isoPath, null);

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
            if (File.Exists(isoPath))
            {
                File.Delete(isoPath);
            }
        }
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
