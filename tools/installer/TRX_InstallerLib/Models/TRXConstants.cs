using TRX_InstallerLib.Utils;

namespace TRX_InstallerLib.Models;

public class TRXConstants
{
    private static readonly string _constConfigPath = "Resources.const.json";

    public static TRXConstants Instance { get; private set; }

    static TRXConstants()
    {
        Instance = JsonUtils.LoadEmbeddedResource(_constConfigPath)?.ToObject<TRXConstants>() ?? new();
    }

    public string? Game { get; set; }
    public bool? AllowExpansionTypeSelection { get; set; }
    
}
