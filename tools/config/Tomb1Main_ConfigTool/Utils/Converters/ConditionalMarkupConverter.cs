using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Markup;

namespace Tomb1Main_ConfigTool.Utils;

public class ConditionalMarkupConverter : MarkupExtension, IValueConverter
{
    public object FalseValue { get; set; }
    public object TrueValue { get; set; }

    public virtual object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is true ? TrueValue : FalseValue;
    }

    public virtual object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotSupportedException();
    }

    public override object ProvideValue(IServiceProvider serviceProvider)
    {
        return this;
    }
}