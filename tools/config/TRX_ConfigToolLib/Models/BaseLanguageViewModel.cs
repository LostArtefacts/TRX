namespace TRX_ConfigToolLib.Models;

public class BaseLanguageViewModel : BaseNotifyPropertyChanged
{
    public static Dictionary<string, string> ViewText
    {
        get => Language.Instance.Controls;
    }
}
