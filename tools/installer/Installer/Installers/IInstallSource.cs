using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Installer.Installers;

public interface IInstallSource
{
    public IEnumerable<string> DirectoriesToTry { get; }
    public string ImageSource { get; }
    public string SourceName { get; }
    public bool SuggestDownloadingMusic { get; }
    public bool SuggestDownloadingUnfinishedBusiness { get; }
    public string SuggestedInstallationDirectory { get; }

    public Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool overwrite
    );

    public bool IsGameFound(string sourceDirectory);
};
