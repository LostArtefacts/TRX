using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Installer.Installers;

public class TR1XInstallSource : BaseInstallSource
{
    public override IEnumerable<string> DirectoriesToTry
    {
        get
        {
            var previousPath = InstallUtils.GetPreviousInstallationPath();
            if (previousPath is not null)
            {
                yield return previousPath;
            }

            foreach (var path in InstallUtils.GetDesktopShortcutDirectories())
            {
                yield return path;
            }
        }
    }

    public override string SuggestedInstallationDirectory
    {
        get
        {
            return InstallUtils.GetPreviousInstallationPath() ?? base.SuggestedInstallationDirectory;
        }
    }

    public override bool IsImportingSavesSupported => true;
    public override string SourceName => "TR1X";

    public override async Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool importSaves
    )
    {
        var filterRegex = new Regex(importSaves ? @"(data|fmv|music|saves)[\\/]|save.*\.\d+" : @"(data|fmv|music)[\\/]", RegexOptions.IgnoreCase);
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
        return File.Exists(Path.Combine(sourceDirectory, "TR1X.exe")) || File.Exists(Path.Combine(sourceDirectory, "Tomb1Main.exe"));
    }
}
