using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace TRX_Installer;

public class OptionalDownload : INotifyPropertyChanged
{
    public OptionalDownload(string title, string url, bool isEnabled = true)
    {
        Title = title;
        Url = url;
        _isEnabled = isEnabled;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Title { get; }
    public string Url { get; }

    public bool IsEnabled
    {
        get => _isEnabled;
        set
        {
            if (value == _isEnabled)
            {
                return;
            }
            _isEnabled = value;
            OnPropertyChanged();
        }
    }

    private bool _isEnabled;

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
}
