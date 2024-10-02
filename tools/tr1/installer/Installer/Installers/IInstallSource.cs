using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Installer.Installers;

public interface IInstallSource
{
    public IEnumerable<string> DirectoriesToTry { get; }

    public string ImageSource { get; }

    public string SourceName { get; }

    public string SuggestedInstallationDirectory { get; }

    public Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool importSaves
    );

    bool IsDownloadingMusicNeeded(string sourceDirectory);

    bool IsDownloadingUnfinishedBusinessNeeded(string sourceDirectory);

    public bool IsGameFound(string sourceDirectory);

    bool IsImportingSavesSupported { get; }
};
