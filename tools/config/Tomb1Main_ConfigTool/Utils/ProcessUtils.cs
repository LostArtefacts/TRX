using System;
using System.Diagnostics;
using System.IO;

namespace Tomb1Main_ConfigTool.Utils;

public static class ProcessUtils
{
    public static void Start(string fileName, string arguments = null)
    {
        Process.Start(new ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            UseShellExecute = true,
            WorkingDirectory = new Uri(fileName).IsFile
                ? Path.GetDirectoryName(fileName)
                : null
        });
    }
}