using System.Windows;
using TR1X_ConfigTool.Models;
using TR1X_ConfigTool.Utils;

namespace TR1X_ConfigTool.Controls;

public partial class AboutWindow : Window
{
    public AboutWindow()
    {
        InitializeComponent();
        DataContext = new BaseLanguageViewModel();
        Owner = Application.Current.MainWindow;
    }

    private void GitHubHyperlink_Click(object sender, RoutedEventArgs e)
    {
        ProcessUtils.Start(TR1XConstants.GitHubURL);
    }
}