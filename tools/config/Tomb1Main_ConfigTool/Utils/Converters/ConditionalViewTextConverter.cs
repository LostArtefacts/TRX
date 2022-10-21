using System;
using System.Globalization;
using Tomb1Main_ConfigTool.Models;

namespace Tomb1Main_ConfigTool.Utils;

public class ConditionalViewTextConverter : ConditionalMarkupConverter
{
    public override object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return Language.Instance.Controls[base.Convert(value, targetType, parameter, culture).ToString()];
    }
}