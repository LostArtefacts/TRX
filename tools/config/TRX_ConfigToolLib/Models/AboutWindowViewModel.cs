using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models;

public class AboutWindowViewModel : BaseLanguageViewModel
{
    public static string ImageSource
    {
        get => AssemblyUtils.GetEmbeddedResourcePath(TRXConstants.Instance.AppImage);
    }
}
