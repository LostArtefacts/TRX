using TRX_ConfigToolLib.Models.Lang;
using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models;

public class BaseLanguageViewModel : BaseNotifyPropertyChanged
{
    public static Dictionary<string, string> ViewText
    {
        get => Language.Instance.Controls;
    }
}
