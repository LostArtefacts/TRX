using System.ComponentModel;

namespace Installer.Models;

public class FinishStep : IStep
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public bool CanProceedToNextStep => false;
    public bool CanProceedToPreviousStep => false;
}
