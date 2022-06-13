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

    public override string SourceName => "TombATI";

    public override async Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool overwrite
    ) {
        var filterRegex = new Regex(@"(data|fmv|music)[\\/]|save.*\.\d+\b", RegexOptions.IgnoreCase);
        await InstallUtils.CopyDirectoryTree(
            sourceDirectory,
            targetDirectory,
            progress,
            file => filterRegex.IsMatch(file),
            file => overwrite
        );
    }

    public override bool IsGameFound(string sourceDirectory)
    {
        return Directory.Exists(Path.Combine(sourceDirectory, "DATA"))
            && Directory.Exists(Path.Combine(sourceDirectory, "FMV"))
            && File.Exists(Path.Combine(sourceDirectory, "TombATI.exe"));
    }
}
