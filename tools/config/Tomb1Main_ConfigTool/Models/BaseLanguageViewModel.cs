using System.Collections.Generic;

namespace Tomb1Main_ConfigTool.Models;

public class BaseLanguageViewModel : BaseNotifyPropertyChanged
{
    public static Dictionary<string, string> ViewText
    {
        get => Language.Instance.Controls;
    }
}