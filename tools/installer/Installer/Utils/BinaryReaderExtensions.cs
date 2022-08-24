using System.IO;
using System.Text;

namespace Installer.Utils;

public static class BinaryReaderExtensions
{
    public static string ReadNullTerminatedString(this BinaryReader stream)
    {
        string str = "";
        char ch;
        while ((int)(ch = stream.ReadChar()) != 0)
        {
            str = str + ch;
        }
        return str;
    }

    public static string ReadSystemCodepageString(this BinaryReader stream)
    {
        var length = stream.ReadUInt16();
        return Encoding.Default.GetString(stream.ReadBytes(length));
    }

    public static string ReadUtf16String(this BinaryReader stream)
    {
        var length = stream.ReadUInt16();
        return Encoding.Unicode.GetString(stream.ReadBytes(length * 2));
    }
}
