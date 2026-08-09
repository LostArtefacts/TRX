using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TRX_Installer;

internal static class Program
{
    private const int MB_OK = 0x00000000;
    private const int MB_ICONERROR = 0x00000010;

    [STAThread]
    private static int Main()
    {
        try
        {
            return RunApp();
        }
        catch (Exception ex)
        {
            ShowMessageBox(0, BuildMessage(ex), "TRX Installer", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    // The app is started from a separate method so that this assembly's WPF
    // types are only touched once Main is running and the handler above is in
    // place. Inlining it would move the type loads back into Main, where a
    // damaged runtime kills the process before anything can report it.
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static int RunApp()
    {
        App app = new();
        app.InitializeComponent();
        return app.Run();
    }

    private static string BuildMessage(Exception ex)
    {
        bool runtimeProblem = ex
            is FileLoadException
            or FileNotFoundException
            or BadImageFormatException
            or TypeLoadException
            or MissingMethodException;

        string summary = runtimeProblem
            ? "The installer could not start because the .NET 8 Desktop Runtime "
                + "is missing or damaged."
                + Environment.NewLine
                + Environment.NewLine
                + "Install the x64 version from "
                + "https://dotnet.microsoft.com/download/dotnet/8.0 and run the "
                + "installer again."
            : "The installer stopped because of an unexpected error.";

        return summary
            + Environment.NewLine
            + Environment.NewLine
            + ex;
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "MessageBoxW")]
    private static extern int ShowMessageBox(
        nint hWnd, string text, string caption, int type);
}
