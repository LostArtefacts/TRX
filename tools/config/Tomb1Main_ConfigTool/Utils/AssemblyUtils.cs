using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Tomb1Main_ConfigTool.Utils;

public static class AssemblyUtils
{
    public static Stream GetResourceStream(string relativePath)
    {
        return Assembly.GetExecutingAssembly().GetManifestResourceStream(GetAbsolutePath(relativePath));
    }

    public static bool ResourceExists(string relativePath)
    {
        List<string> resources = Assembly.GetExecutingAssembly().GetManifestResourceNames().ToList();
        return resources.Contains(GetAbsolutePath(relativePath));
    }

    public static string GetAbsolutePath(string relativePath)
    {
        return Assembly.GetEntryAssembly().GetName().Name + "." + relativePath;
    }
}