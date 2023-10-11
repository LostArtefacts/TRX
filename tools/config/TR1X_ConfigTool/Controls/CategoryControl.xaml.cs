using System.Windows.Controls;
using TR1X_ConfigTool.Models;

namespace TR1X_ConfigTool.Controls;

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
