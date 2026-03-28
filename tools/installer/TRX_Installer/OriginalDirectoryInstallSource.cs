namespace TRX_Installer;

internal class OriginalDirectoryInstallSource : IInstallSource
{
    public async Task InstallAsync(
        string sourceDirectory,
        string targetDirectory,
        string gameId,
        IInstallerProgress progress)
    {
        await InstallFileHelper.CopyMappedDirectoryAsync(
            sourceDirectory,
            targetDirectory,
            (relPath, availablePaths) => InstallMappings.MapOriginalFile(gameId, relPath, availablePaths),
            progress);
    }
}
