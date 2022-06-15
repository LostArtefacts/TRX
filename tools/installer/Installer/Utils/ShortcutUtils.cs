using IWshRuntimeLibrary;
using System.Linq;
using System.Text.RegularExpressions;

namespace Installer.Utils;

public static class ShortcutUtils
{
    public static void CreateShortcut(string shortcutPath, string targetPath, string name, string[]? args = null)
    {
        var shell = new WshShell();
        var shortcut = (IWshShortcut)shell.CreateShortcut(shortcutPath);
        shortcut.Description = name;
        shortcut.TargetPath = targetPath;

        if (args is not null)
        {
            shortcut.Arguments = string.Join(
                " ",
                args.Select(
                    arg =>
                    {
                        if (string.IsNullOrEmpty(arg))
                            return arg;
                        string value = Regex.Replace(arg, @"(\\*)" + "\"", @"$1\$0");
                        value = Regex.Replace(value, @"^(.*\s.*?)(\\*)$", "\"$1$2$2\"");
                        return value;
                    }
                ).ToArray()
            );
        }

        shortcut.Save();
    }

    public static string? GetLnkTargetPath(string filepath)
    {
        var shell = new WshShellClass();
        var shortcut = (IWshShortcut)shell.CreateShortcut(filepath);
        return shortcut.TargetPath;
    }
}
