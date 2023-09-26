using System.Windows;

namespace TR1X_ConfigTool.Utils;

public class BindingProxy : Freezable
{
    public static readonly DependencyProperty DataProperty = DependencyProperty.Register
    (
        nameof(Data), typeof(object), typeof(BindingProxy), new UIPropertyMetadata(null)
    );

    public object Data
    {
        get => GetValue(DataProperty);
        set => SetValue(DataProperty, value);
    }

    protected override Freezable CreateInstanceCore()
    {
        return new BindingProxy();
    }
}