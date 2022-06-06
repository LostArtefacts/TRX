using System.ComponentModel;

namespace Installer.Models;

public interface IStep : INotifyPropertyChanged
{
    bool CanProceedToNextStep { get; }
    bool CanProceedToPreviousStep { get; }
}
