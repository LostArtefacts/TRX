using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Installer.Installers;

public class Tomb1MainInstallSource : BaseInstallSource
{
    public override IEnumerable<string> DirectoriesToTry
    {
        get
        {
            using var key = Registry.CurrentUser.OpenSubKey(@"Software\Tomb1Main");
            if (key is not null)
            {
                var value = key.GetValue("InstallPath")?.ToString();
                if (value is not null)
                {
                    yield return value;
                }
            }

            foreach (var path in InstallUtils.GetDesktopShortcutDirectories())
            {
                yield return path;
            }
        }
    }

    public override string SourceName => "Tomb1Main";

    public override bool SuggestImportingSaves
    {
        get
        {
            return true;
        }
    }

    public override async Task CopyOriginalGameFiles(
        string sourceDirectory,
        string targetDirectory,
        IProgress<InstallProgress> progress,
        bool importSaves,
        bool overwrite
    )
    {
        var filterRegex = new Regex(importSaves ? @"(data|fmv|music|saves)[\\/]|save.*\.\d+" : @"(data|fmv|music)[\\/]", RegexOptions.IgnoreCase);
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
        return File.Exists(Path.Combine(sourceDirectory, "Tomb1Main.exe"));
    }
}
