using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;

namespace Installer.Installers;

public abstract class BaseInstallSource : IInstallSource
{
    public abstract IEnumerable<string> DirectoriesToTry { get; }

    public virtual string ImageSource
    {
        get
        {
            return $"pack://application:,,,/TR1X_Installer;component/Resources/{SourceName}.png";
        }
    }

    public abstract bool IsImportingSavesSupported { get; }
    public abstract string SourceName { get; }

    public virtual string SuggestedInstallationDirectory
    {
        get
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "TR1X");
        }
    }

    public abstract Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool importSaves
    );

    public abstract bool IsDownloadingMusicNeeded(string sourceDirectory);

    public abstract bool IsDownloadingUnfinishedBusinessNeeded(string sourceDirectory);

    public abstract bool IsGameFound(string sourceDirectory);
}
