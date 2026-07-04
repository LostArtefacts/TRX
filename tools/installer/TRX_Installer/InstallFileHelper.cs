using System.IO;

namespace TRX_Installer;

internal static class InstallFileHelper
{
    public static async Task CopyMappedDirectoryAsync(
        string sourceDirectory,
        string targetDirectory,
        Func<string, ISet<string>, string?> mapPath,
        IInstallerProgress progress)
    {
        string[] files = Directory.GetFiles(sourceDirectory, "*", SearchOption.AllDirectories);
        HashSet<string> relativePaths = files
            .Select(sourcePath => InstallMappings.Normalize(Path.GetRelativePath(sourceDirectory, sourcePath)))
            .ToHashSet(StringComparer.OrdinalIgnoreCase);
        int copied = 0;
        progress.SetSubDescription($"Copying files from {Path.GetFileName(sourceDirectory)}...");
        progress.SetInnerProgress(0, Math.Max(1, files.Length));

        foreach (string sourcePath in files)
        {
            string relPath = Path.GetRelativePath(sourceDirectory, sourcePath);
            string? mappedPath = mapPath(relPath, relativePaths);
            if (mappedPath is null)
            {
                copied++;
                progress.SetInnerProgress(copied, Math.Max(1, files.Length));
                continue;
            }

            string targetPath = Path.Combine(targetDirectory, mappedPath);
            string fullSourcePath = Path.GetFullPath(sourcePath);
            string fullTargetPath = Path.GetFullPath(targetPath);
            if (string.Equals(fullSourcePath, fullTargetPath, StringComparison.OrdinalIgnoreCase))
            {
                copied++;
                progress.SetInnerProgress(copied, Math.Max(1, files.Length));
                continue;
            }

            Directory.CreateDirectory(Path.GetDirectoryName(targetPath)!);
            try
            {
                await Task.Run(() => File.Copy(sourcePath, targetPath, true));
                progress.AppendLog($"Copying {mappedPath}");
            }
            catch (IOException ex)
            {
                progress.AppendLog($"Skipping {mappedPath}: {ex.Message}");
            }
            copied++;
            progress.SetInnerProgress(copied, Math.Max(1, files.Length));
        }
    }

    public static async Task DeleteDirectory(string path, bool recursive)
    {
        foreach (var file in Directory.EnumerateFiles(path))
        {
            try
            {
                File.SetAttributes(file, FileAttributes.Normal);
            }
            catch { }
            try
            {
                File.Delete(file);
            }
            catch { }
        }

        if (recursive)
        {
            foreach (var subDir in Directory.EnumerateDirectories(path))
            {
                await DeleteDirectory(subDir, true);
            }
        }

        try
        {
            Directory.Delete(path, false);
        }
        catch { }
    }
}
