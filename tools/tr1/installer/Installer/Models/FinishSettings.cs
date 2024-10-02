namespace Installer.Models;

public class FinishSettings : BaseNotifyPropertyChanged
{
    public bool LaunchGame
    {
        get => _launchGame;
        set
        {
            if (value != _launchGame)
            {
                _launchGame = value;
                NotifyPropertyChanged();
            }
        }
    }

    public bool OpenGameDirectory
    {
        get => _openGameDirectory;
        set
        {
            if (value != _openGameDirectory)
            {
                _openGameDirectory = value;
                NotifyPropertyChanged();
            }
        }
    }

    private bool _launchGame = false;
    private bool _openGameDirectory = true;
}
