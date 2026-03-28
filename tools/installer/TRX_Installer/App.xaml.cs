using System.Windows;

namespace TRX_Installer;

public partial class App : System.Windows.Application
{
    public App()
    {
        Current.MainWindow = new MainWindow();
        Current.MainWindow.Show();
    }
}
