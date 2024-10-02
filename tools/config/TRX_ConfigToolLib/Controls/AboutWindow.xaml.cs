using System.Windows;
using TRX_ConfigToolLib.Models;
using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Controls;

public partial class AboutWindow : Window
{
    public AboutWindow()
    {
        InitializeComponent();
        DataContext = new AboutWindowViewModel();
        Owner = Application.Current.MainWindow;
    }

    private void GitHubHyperlink_Click(object sender, RoutedEventArgs e)
    {
        ProcessUtils.Start(TRXConstants.Instance.GitHubURL);
    }
}
