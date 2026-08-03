using System.IO;
using System.Net.Http;

namespace TRX_Installer;

// A download that did not finish, described in terms of what the person running
// the installer can act on. The framework exception it wraps stays available for
// the log.
internal class DownloadFailedException : Exception
{
    public DownloadFailedException(string url, Exception innerException)
        : base(BuildMessage(url, innerException), innerException)
    {
    }

    private static string BuildMessage(string url, Exception innerException)
    {
        string reason = innerException switch
        {
            HttpRequestException { StatusCode: not null } ex
                => $"the server answered with {(int)ex.StatusCode} ({ex.StatusCode}).",
            TaskCanceledException => "the connection timed out.",
            _ => "the installer could not reach the server.",
        };

        return $"Could not download {Path.GetFileName(url)}: {reason}"
            + Environment.NewLine
            + Environment.NewLine
            + "The installer downloads part of what it installs, so it needs "
            + "internet access. Check that you are online and that no firewall "
            + "or antivirus is stopping the installer from connecting, then "
            + "press Retry."
            + Environment.NewLine
            + Environment.NewLine
            + $"The file it was fetching is {url}";
    }
}
