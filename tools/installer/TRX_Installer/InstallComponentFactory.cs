using Microsoft.Win32;
using System.IO;
using System.Text.RegularExpressions;

namespace TRX_Installer;

internal static class InstallComponentFactory
{
    private const string RegistryPath = @"Software\TRX";
    private const string ResourceBaseUrl = "https://lostartefacts.dev/aux/trx";

    private static readonly IInstallSource _originalDir = new OriginalDirectoryInstallSource();
    private static readonly IInstallSource _discImage = new DiscImageInstallSource();
    private static readonly IInstallSource _existingTRX = new ExistingTRXInstallSource();

    public static IEnumerable<InstallComponent> CreateComponents()
    {
        yield return new InstallComponent(
            "tr1",
            "TR1",
            "Original Tomb Raider I files from Steam, GOG, disc, or an existing TRX installation.",
            downloadUrl: $"{ResourceBaseUrl}/tr1.zip",
            sourceOptions:
            [
                new InstallSourceOption(
                    "Steam",
                    GetSteamDirectories("Tomb Raider (I)"),
                    _discImage,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "GAME.GOG"))
                        || File.Exists(Path.Combine(sourceDirectory, "game.dat"))),
                new InstallSourceOption(
                    "GOG",
                    GetGOGDirectories("Tomb Raider 1"),
                    _discImage,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "GAME.GOG"))
                        || File.Exists(Path.Combine(sourceDirectory, "game.dat"))),
                new InstallSourceOption(
                    "Disc",
                    DriveInfo.GetDrives()
                        .Where(drive => drive.DriveType == DriveType.CDRom && drive.IsReady)
                        .Select(drive => drive.RootDirectory.FullName),
                    _originalDir,
                    sourceDirectory =>
                        Directory.Exists(Path.Combine(sourceDirectory, "DATA"))
                        && Directory.Exists(Path.Combine(sourceDirectory, "FMV"))
                        && File.Exists(Path.Combine(sourceDirectory, "dos4gw.exe"))
                        && File.Exists(Path.Combine(sourceDirectory, "tomb.exe"))),
                new InstallSourceOption(
                    "TombATI",
                    GetTombATIDirectories(),
                    _originalDir,
                    sourceDirectory =>
                        Directory.Exists(Path.Combine(sourceDirectory, "DATA"))
                        && Directory.Exists(Path.Combine(sourceDirectory, "FMV"))
                        && File.Exists(Path.Combine(sourceDirectory, "TombATI.exe"))),
                new InstallSourceOption(
                    "Legacy TR1X/TRX",
                    GetTRXDirectories(),
                    _originalDir,
                    sourceDirectory =>
                        Directory.Exists(Path.Combine(sourceDirectory, "cfg"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "level1.phd"))),
                new InstallSourceOption(
                    "TRX",
                    GetTRXDirectories(),
                    _existingTRX,
                    sourceDirectory => Directory.Exists(Path.Combine(sourceDirectory, "games", "tr1")))
            ],
            optionalDownloads:
            [
                new OptionalDownload("Download music", $"{ResourceBaseUrl}/tr1-music.zip"),
            ]);

        yield return new InstallComponent(
            "tr1-ub",
            "TR1:UB",
            "Unfinished Business pack downloaded from Lost Artefacts.",
            downloadUrl: $"{ResourceBaseUrl}/tr1-ub.zip",
            downloadOptions:
            [
                new DownloadOption("Fan-patched edition", $"{ResourceBaseUrl}/tr1-ub-music.zip"),
                new DownloadOption("Original edition", $"{ResourceBaseUrl}/tr1-ub-vanilla.zip"),
            ]);

        yield return new InstallComponent(
            "tr1-demo-pc",
            "TR1 PC Demo",
            "PC demo pack downloaded from Lost Artefacts.",
            downloadUrl: $"{ResourceBaseUrl}/tr1-demo-pc.zip");

        yield return new InstallComponent(
            "tr2",
            "TR2",
            "Original Tomb Raider II files from Steam, GOG, disc, or an existing TRX installation.",
            downloadUrl: $"{ResourceBaseUrl}/tr2.zip",
            sourceOptions:
            [
                new InstallSourceOption(
                    "Steam",
                    GetSteamDirectories("Tomb Raider (II)"),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "tomb2.exe"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "wall.tr2"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "main.sfx"))),
                new InstallSourceOption(
                    "GOG",
                    GetGOGDirectories("Tomb Raider 2"),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "tomb2.exe"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "wall.tr2"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "main.sfx"))),
                new InstallSourceOption(
                    "Disc",
                    DriveInfo.GetDrives()
                        .Where(drive => drive.DriveType == DriveType.CDRom && drive.IsReady)
                        .Select(drive => drive.RootDirectory.FullName),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "fmv", "ancient.rpl"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "wall.tr2"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "main.sfx"))),
                new InstallSourceOption(
                    "Legacy TR2X/TRX",
                    GetTRXDirectories(),
                    _originalDir,
                    sourceDirectory =>
                        Directory.Exists(Path.Combine(sourceDirectory, "cfg"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "wall.tr2"))),
                new InstallSourceOption(
                    "TRX",
                    GetTRXDirectories(),
                    _existingTRX,
                    sourceDirectory => Directory.Exists(Path.Combine(sourceDirectory, "games", "tr2")))
            ],
            optionalDownloads:
            [
                new OptionalDownload("Download music", $"{ResourceBaseUrl}/tr2-music.zip"),
            ]);

        yield return new InstallComponent(
            "tr2-gm",
            "TR2:GM",
            "Golden Mask pack downloaded from Lost Artefacts.",
            downloadUrl: $"{ResourceBaseUrl}/tr2-gm.zip");

        yield return new InstallComponent(
            "tr3",
            "TR3",
            "Original Tomb Raider III files from Steam, GOG, disc, or an existing TRX installation.",
            downloadUrl: $"{ResourceBaseUrl}/tr3.zip",
            sourceOptions:
            [
                new InstallSourceOption(
                    "Steam",
                    GetSteamDirectories("Tomb Raider (III)", "TombRaider (III)"),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "data", "jungle.tr2"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "main.sfx"))
                        && File.Exists(Path.Combine(sourceDirectory, "audio", "cdaudio.wad"))),
                new InstallSourceOption(
                    "GOG",
                    GetGOGDirectories("Tomb Raider 3"),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "data", "jungle.tr2"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "main.sfx"))
                        && File.Exists(Path.Combine(sourceDirectory, "audio", "cdaudio.wad"))),
                new InstallSourceOption(
                    "Disc",
                    DriveInfo.GetDrives()
                        .Where(drive => drive.DriveType == DriveType.CDRom && drive.IsReady)
                        .Select(drive => drive.RootDirectory.FullName),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "data", "jungle.tr2"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "main.sfx"))
                        && File.Exists(Path.Combine(sourceDirectory, "audio", "cdaudio.wad"))),
                new InstallSourceOption(
                    "Legacy TR3X/TRX",
                    GetTRXDirectories(),
                    _originalDir,
                    sourceDirectory =>
                        Directory.Exists(Path.Combine(sourceDirectory, "cfg"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "jungle.tr2"))),
                new InstallSourceOption(
                    "TRX",
                    GetTRXDirectories(),
                    _existingTRX,
                    sourceDirectory => Directory.Exists(Path.Combine(sourceDirectory, "games", "tr3")))
            ]);

        yield return new InstallComponent(
            "tr3-la",
            "TR3:LA",
            "The Lost Artifact files from disc or an existing TRX installation.",
            downloadUrl: $"{ResourceBaseUrl}/tr3-la.zip",
            sourceOptions:
            [
                new InstallSourceOption(
                    "Steam",
                    GetSteamDirectories("Tomb Raider (III)", "TombRaider (III)"),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "tr3gold.exe"))
                        || File.Exists(Path.Combine(sourceDirectory, "data", "trtla.dat"))),
                new InstallSourceOption(
                    "GOG",
                    GetGOGDirectories("Tomb Raider 3"),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "tr3gold.exe"))
                        || File.Exists(Path.Combine(sourceDirectory, "data", "trtla.dat"))),
                new InstallSourceOption(
                    "Disc",
                    DriveInfo.GetDrives()
                        .Where(drive => drive.DriveType == DriveType.CDRom && drive.IsReady)
                        .Select(drive => drive.RootDirectory.FullName),
                    _originalDir,
                    sourceDirectory =>
                        File.Exists(Path.Combine(sourceDirectory, "tr3gold.exe"))
                        || File.Exists(Path.Combine(sourceDirectory, "data", "trtla.dat"))),
                new InstallSourceOption(
                    "Legacy TR3X/TRX",
                    GetTRXDirectories(),
                    _originalDir,
                    sourceDirectory =>
                        Directory.Exists(Path.Combine(sourceDirectory, "cfg"))
                        && File.Exists(Path.Combine(sourceDirectory, "data", "chunnel.tr2"))),
                new InstallSourceOption(
                    "TRX",
                    GetTRXDirectories(),
                    _existingTRX,
                    sourceDirectory => Directory.Exists(Path.Combine(sourceDirectory, "games", "tr3-la"))),
            ]);
    }

    public static string? GetStoredInstallPath()
    {
        using RegistryKey? key = Registry.CurrentUser.OpenSubKey(RegistryPath);
        return key?.GetValue("InstallPath")?.ToString();
    }

    public static void StoreInstallPath(string installPath)
    {
        using RegistryKey? key = Registry.CurrentUser.CreateSubKey(RegistryPath);
        key?.SetValue("InstallPath", installPath);
    }

    private static IEnumerable<string> GetSteamDirectories(params string[] gameDirectoryNames)
    {
        foreach (string gameDirectoryName in gameDirectoryNames)
        {
            yield return Path.Combine(@"C:\Program Files (x86)\Steam\steamapps\common", gameDirectoryName);
        }

        using RegistryKey? key = Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam");
        string? steamPath = key?.GetValue("SteamPath")?.ToString();
        if (steamPath is not null)
        {
            foreach (string gameDirectoryName in gameDirectoryNames)
            {
                yield return Path.Combine(steamPath, @"steamapps\common", gameDirectoryName);
            }
        }
    }

    private static IEnumerable<string> GetGOGDirectories(string gameDirectoryName)
    {
        yield return Path.Combine(@"C:\Program Files (x86)\GOG Galaxy\Games", gameDirectoryName);
        using RegistryKey? key = Registry.ClassesRoot.OpenSubKey(@"goggalaxy\shell\open\command");
        string? value = key?.GetValue("")?.ToString();
        if (value is null)
        {
            yield break;
        }
        Match match = new Regex(@"""(?<path>[^""]+)""").Match(value);
        if (!match.Success)
        {
            yield break;
        }
        string? gogPath = Path.GetDirectoryName(match.Groups["path"].Value);
        if (gogPath is not null)
        {
            yield return Path.Combine(gogPath, @"Games", gameDirectoryName);
        }
    }

    private static IEnumerable<string> GetTRXDirectories()
    {
        string? path = GetStoredInstallPath();
        if (path is not null)
        {
            yield return path;
        }
    }

    private static IEnumerable<string> GetTombATIDirectories()
    {
        yield return @"C:\TOMBATI";

        foreach (string path in GetDesktopShortcutDirectories())
        {
            yield return path;
        }

        foreach (string path in GetSteamDirectories("Tomb Raider (I)"))
        {
            yield return path;
        }
    }

    private static IEnumerable<string> GetDesktopShortcutDirectories()
    {
        string desktopDir = Environment.GetFolderPath(
            Environment.SpecialFolder.DesktopDirectory);
        if (!Directory.Exists(desktopDir))
        {
            yield break;
        }

        foreach (string shortcutPath in Directory.EnumerateFiles(desktopDir, "*.lnk"))
        {
            string? targetPath = null;
            try
            {
                Type? shellType = Type.GetTypeFromProgID("WScript.Shell");
                if (shellType == null)
                {
                    continue;
                }

                dynamic shell = Activator.CreateInstance(shellType)!;
                dynamic shortcut = shell.CreateShortcut(shortcutPath);
                targetPath = shortcut.TargetPath as string;
            }
            catch
            {
                continue;
            }

            if (string.IsNullOrWhiteSpace(targetPath))
            {
                continue;
            }

            string? dirName = Path.GetDirectoryName(targetPath);
            if (!string.IsNullOrWhiteSpace(dirName))
            {
                yield return dirName;
            }
        }
    }
}
