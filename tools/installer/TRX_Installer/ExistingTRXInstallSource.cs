namespace TRX_Installer;

internal class ExistingTRXInstallSource : IInstallSource
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
            (relPath, _) => InstallMappings.MapCombinedFile(gameId, relPath),
            progress);
    }
}
