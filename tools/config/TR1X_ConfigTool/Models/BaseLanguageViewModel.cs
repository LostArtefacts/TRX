using System.Collections.Generic;

namespace TR1X_ConfigTool.Models;

public class BaseLanguageViewModel : BaseNotifyPropertyChanged
{
    public static Dictionary<string, string> ViewText
    {
        get => Language.Instance.Controls;
    }
}