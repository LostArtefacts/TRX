namespace Installer.Models;

public class FinishStep : BaseNotifyPropertyChanged, IStep
{
    public FinishStep(FinishSettings finishSettings)
    {
        FinishSettings = finishSettings;
    }

    public bool CanProceedToNextStep => false;
    public bool CanProceedToPreviousStep => false;
    public FinishSettings FinishSettings { get; }
    public string SidebarImage => "pack://application:,,,/TR1X_Installer;component/Resources/side4.jpg";
}
