namespace TRX_Installer;

public class DownloadOption
{
    public DownloadOption(string title, string url)
    {
        Title = title;
        Url = url;
    }

    public string Title { get; }
    public string Url { get; }
}
