using System.Windows.Forms;

namespace Installer.Utils;

public class FileBrowser
{
    public static string? Browse(string? initialDirectory)
    {
        using var dlg = new FolderBrowserDialog()
        {
            Description = "Choose directory",
            SelectedPath = initialDirectory,
            ShowNewFolderButton = true,
        };
        if (dlg.ShowDialog() == DialogResult.OK)
        {
            return dlg.SelectedPath;
        }
        return null;
    }
}
