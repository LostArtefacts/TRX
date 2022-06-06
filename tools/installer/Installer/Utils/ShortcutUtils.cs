using System;
using System.IO;
using System.Text;

namespace Installer.Utils;

public static class ShortcutUtils
{
    public static void CreateShortcut(string path, string shortcutPath, string description)
    {
        using var writer = new StreamWriter(Path.ChangeExtension(shortcutPath, "url"));
        writer.WriteLine("[InternetShortcut]");
        writer.WriteLine("URL=file:///" + path);
        writer.WriteLine("IconIndex=0");
        var icon = path.Replace('\\', '/');
        writer.WriteLine("IconFile=" + icon);
    }

    /// <summary>
    /// .NET Core compatible .lnk reader.
    /// MS Documentation:
    /// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/16cb4ca1-9339-4d0c-a68d-bf1d6cc0f943?redirectedfrom=MSDN
    /// </summary>
    public static string? GetLnkTargetPath(string filepath)
    {
        using var br = new BinaryReader(File.OpenRead(filepath));

        var headerSize = br.ReadUInt32();
        if (headerSize != 0x4C)
        {
            throw new ApplicationException("Invalid LNK signature");
        }

        br.ReadBytes(0x10); // skip LinkCLSID

        // LinkFlags
        var linkFlags = br.ReadUInt32();

        br.ReadBytes(4); // skip FileAttributes
        br.ReadBytes(8); // skip CreationTime
        br.ReadBytes(8); // skip AccessTime
        br.ReadBytes(8); // skip WriteTime
        br.ReadBytes(4); // skip FileSize
        br.ReadBytes(4); // skip IconIndex
        br.ReadBytes(4); // skip ShowCommand
        br.ReadBytes(2); // skip Hotkey
        br.ReadBytes(2); // skip Reserved
        br.ReadBytes(4); // skip Reserved2
        br.ReadBytes(4); // skip Reserved3

        // if the HasLinkTargetIDList bit, skip LinkTargetIDList
        if ((linkFlags & 0x01) == 0x01)
        {
            var skip = br.ReadUInt16();
            br.ReadBytes(skip);
        }

        if ((linkFlags & 0x02) == 0x02)
        {
            // get the number of bytes the path contains
            var linkInfoSize = br.ReadUInt32();
            br.ReadBytes(4); // skip LinkInfoHeaderSize
            br.ReadBytes(4); // skip LinkInfoFlags
            br.ReadBytes(4); // skip VolumeIDOffset

            // Find the location of the LocalBasePath position
            var localPathBaseOffset = br.ReadUInt32();
            // Skip to the path position
            // (subtract the length of the read (4 bytes), the length of the skip (12 bytes), and
            // the length of the localPathBaseOffset read (4 bytes) from the localPathBaseOffset)
            br.ReadBytes((int)localPathBaseOffset - 0x14);
            var size = linkInfoSize - localPathBaseOffset - 0x02;
            var bytePath = br.ReadBytes((int)size);
            var path = Encoding.UTF8.GetString(bytePath, 0, bytePath.Length);
            return path;
        }

        if ((linkFlags & 0x04) == 0x04)
        {
            br.ReadUtf16String(); // skip Name
        }
        string? relativePath = null;
        if ((linkFlags & 0x08) == 0x08)
        {
            relativePath = br.ReadUtf16String();
        }
        string? workingDir = null;
        if ((linkFlags & 0x10) == 0x10)
        {
            workingDir = br.ReadUtf16String();
        }

        if (workingDir is not null && relativePath is not null)
        {
            return Path.Combine(workingDir, relativePath);
        }
        if (workingDir is not null)
        {
            return workingDir;
        }
        if (relativePath is not null)
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), relativePath);
        }

        throw new ApplicationException("Unable to determine link taret path");
    }
}
