using System.Windows;

namespace TR1X_ConfigTool.Utils;

public static class MessageBoxUtils
{
    public static MessageBoxResult ShowYesNoCancel(string message, string caption)
    {
        return MessageBox.Show(message, caption, MessageBoxButton.YesNoCancel, MessageBoxImage.Question);
    }

    public static bool ShowYesNo(string message, string caption)
    {
        return MessageBox.Show(message, caption, MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes;
    }

    public static void ShowError(string message, string caption)
    {
        MessageBox.Show(message, caption, MessageBoxButton.OK, MessageBoxImage.Error);
    }
}