using System.Diagnostics;

namespace Tomb1Main_ConfigTool.Utils;

public static class ProcessUtils
{
    public static void Start(string fileName)
    {
        Process.Start(new ProcessStartInfo
        {
            FileName = fileName,
            UseShellExecute = true
        });
    }
}