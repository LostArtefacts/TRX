using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Markup;

namespace Installer.Utils;

public sealed class ConditionalMarkupConverter : MarkupExtension, IValueConverter
{
    public object FalseValue { get; set; } = new();
    public object TrueValue { get; set; } = new();

    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is true ? TrueValue : FalseValue;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotSupportedException();
    }

    public override object ProvideValue(IServiceProvider serviceProvider)
    {
        return this;
    }
}
