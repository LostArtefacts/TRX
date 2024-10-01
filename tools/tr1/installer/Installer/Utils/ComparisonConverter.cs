using System;
using System.Globalization;
using System.Windows.Data;

namespace Installer.Utils;

public class ComparisonConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value.Equals(parameter);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return (bool)value ? parameter : Binding.DoNothing;
    }
}
