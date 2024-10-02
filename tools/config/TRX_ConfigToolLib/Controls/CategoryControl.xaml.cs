using System.Windows.Controls;
using TRX_ConfigToolLib.Models;

namespace TRX_ConfigToolLib.Controls;

public partial class CategoryControl : UserControl
{
    public CategoryControl()
    {
        InitializeComponent();
    }

    private void ScrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
    {
        (DataContext as CategoryViewModel).ViewPosition = e.VerticalOffset;
    }
}
