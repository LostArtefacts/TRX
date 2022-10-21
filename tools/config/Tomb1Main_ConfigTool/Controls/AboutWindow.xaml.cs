using System.Windows;
using Tomb1Main_ConfigTool.Models;
using Tomb1Main_ConfigTool.Utils;

namespace Tomb1Main_ConfigTool.Controls;

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
        ProcessUtils.Start(Tomb1MainConstants.GitHubURL);
    }
}