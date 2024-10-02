using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models;

public class TRXConstants
{
    private static readonly string _constConfigPath = "Resources.const.json";

    public static TRXConstants Instance { get; private set; }

    static TRXConstants()
    {
        Instance = JsonUtils.LoadEmbeddedResource(_constConfigPath).ToObject<TRXConstants>();
    }

    public string AppImage { get; set; }
    public string ConfigFilterExtension { get; set; }
    public string ConfigPath { get; set; }
    public string ExecutableName { get; set; }
    public bool GoldSupported { get; set; }
    public string GoldArgs { get; set; }
    public string GitHubURL { get; set; }
}
