using System.Globalization;
using TRX_ConfigToolLib.Models.Lang;

namespace TRX_ConfigToolLib.Utils.Converters;

public class ConditionalViewTextConverter : ConditionalMarkupConverter
{
    public override object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return Language.Instance.Controls[base.Convert(value, targetType, parameter, culture).ToString()];
    }
}
