using System.Windows.Controls;
using Tomb1Main_ConfigTool.Models;

namespace Tomb1Main_ConfigTool.Controls;

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