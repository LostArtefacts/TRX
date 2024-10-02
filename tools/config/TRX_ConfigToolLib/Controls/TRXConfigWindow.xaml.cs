using System.ComponentModel;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Markup;
using TRX_ConfigToolLib.Models;
using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib;

public partial class TRXConfigWindow : Window
{
    static TRXConfigWindow()
    {
        LanguageProperty.OverrideMetadata
        (
            typeof(FrameworkElement),
            new FrameworkPropertyMetadata(XmlLanguage.GetLanguage(CultureInfo.CurrentCulture.Name))
        );
    }

    public TRXConfigWindow()
    {
        InitializeComponent();
        DataContext = new MainWindowViewModel();
    }

    private void Window_Closing(object sender, CancelEventArgs e)
    {
        (DataContext as MainWindowViewModel).Exit(e);
    }

    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
        (DataContext as MainWindowViewModel).Load();
    }

    private void TabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (e.AddedItems.Count > 0
            && e.AddedItems[0] is CategoryViewModel
            && VisualUtils.GetChild(sender as DependencyObject, typeof(ScrollViewer)) is ScrollViewer scroller)
        {
            scroller.ScrollToVerticalOffset((DataContext as MainWindowViewModel).SelectedCategory.ViewPosition);
        }
    }
}
