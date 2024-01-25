using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Installer.Installers;

public class CDRomInstallSource : BaseInstallSource
{
    public override IEnumerable<string> DirectoriesToTry
    {
        get
        {
            DriveInfo[] allDrives = DriveInfo.GetDrives();
            foreach (var drive in allDrives)
            {
                if (drive.DriveType == DriveType.CDRom && drive.IsReady)
                {
                    yield return drive.RootDirectory.FullName;
                }
            }
        }
    }

    public override bool IsImportingSavesSupported => false;
    public override string SourceName => "CDRom";

    public override async Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool importSaves
    )
    {
        var filterRegex = new Regex(@"(data|fmv|music)[\\/]", RegexOptions.IgnoreCase);
        await InstallUtils.CopyDirectoryTree(
            sourceDirectory,
            targetDirectory,
            progress,
            file => filterRegex.IsMatch(file)
        );
    }

    public override bool IsDownloadingMusicNeeded(string sourceDirectory)
    {
        return true;
    }

    public override bool IsDownloadingUnfinishedBusinessNeeded(string sourceDirectory)
    {
        return true;
    }

    public override bool IsGameFound(string sourceDirectory)
    {
        return Directory.Exists(Path.Combine(sourceDirectory, "DATA"))
            && Directory.Exists(Path.Combine(sourceDirectory, "FMV"))
            && File.Exists(Path.Combine(sourceDirectory, "dos4gw.exe"))
            && File.Exists(Path.Combine(sourceDirectory, "tomb.exe"));
    }
}
