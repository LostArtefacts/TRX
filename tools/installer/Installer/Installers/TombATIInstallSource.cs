using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Installer.Installers;

public class TombATIInstallSource : BaseInstallSource
{
    public override IEnumerable<string> DirectoriesToTry
    {
        get
        {
            yield return "C:\\TOMBATI";
            foreach (var path in InstallUtils.GetDesktopShortcutDirectories())
            {
                yield return path;
            }
            foreach (var path in new SteamInstallSource().DirectoriesToTry)
            {
                yield return path;
            }
        }
    }

    public override bool IsImportingSavesSupported => true;
    public override string SourceName => "TombATI";

    public override async Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool importSaves
    )
    {
        var filterRegex = new Regex(importSaves ? @"(data|fmv|music)[\\/]|save.*\.\d+\b" : @"(data|fmv|music)[\\/]", RegexOptions.IgnoreCase);
        await InstallUtils.CopyDirectoryTree(
            sourceDirectory,
            targetDirectory,
            progress,
            file => filterRegex.IsMatch(file)
        );
    }

    public override bool IsDownloadingMusicNeeded(string sourceDirectory)
    {
        return !Directory.Exists(Path.Combine(sourceDirectory, "music"));
    }

    public override bool IsDownloadingUnfinishedBusinessNeeded(string sourceDirectory)
    {
        return !File.Exists(Path.Combine(sourceDirectory, "data", "cat.phd"));
    }

    public override bool IsGameFound(string sourceDirectory)
    {
        return Directory.Exists(Path.Combine(sourceDirectory, "DATA"))
            && Directory.Exists(Path.Combine(sourceDirectory, "FMV"))
            && File.Exists(Path.Combine(sourceDirectory, "TombATI.exe"));
    }
}
