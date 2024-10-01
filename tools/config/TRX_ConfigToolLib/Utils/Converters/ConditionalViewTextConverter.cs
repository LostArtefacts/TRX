using System.Globalization;
using TRX_ConfigToolLib.Models;

namespace TRX_ConfigToolLib.Utils;

public class ConditionalViewTextConverter : ConditionalMarkupConverter
{
    public override object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return Language.Instance.Controls[base.Convert(value, targetType, parameter, culture).ToString()];
    }
}
