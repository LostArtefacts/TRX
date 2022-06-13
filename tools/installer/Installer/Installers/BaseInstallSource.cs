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
            return $"pack://application:,,,/Tomb1Main_Installer;component/Resources/{SourceName}.png";
        }
    }

    public abstract string SourceName { get; }

    public virtual bool SuggestDownloadingMusic
    {
        get
        {
            return false;
        }
    }

    public virtual bool SuggestDownloadingUnfinishedBusiness
    {
        get
        {
            return false;
        }
    }

    public string SuggestedInstallationDirectory
    {
        get
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "Tomb1Main");
        }
    }

    public abstract Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool overwrite
    );

    public abstract bool IsGameFound(string sourceDirectory);
}
