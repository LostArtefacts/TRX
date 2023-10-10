using System;
using System.Globalization;
using TR1X_ConfigTool.Models;

namespace TR1X_ConfigTool.Utils;

public class ConditionalViewTextConverter : ConditionalMarkupConverter
{
    public override object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return Language.Instance.Controls[base.Convert(value, targetType, parameter, culture).ToString()];
    }
}
