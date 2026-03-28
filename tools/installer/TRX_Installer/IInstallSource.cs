namespace TRX_Installer;

public interface IInstallSource
{
    Task InstallAsync(
        string sourceDirectory,
        string targetDirectory,
        string gameId,
        IInstallerProgress progress);
}
