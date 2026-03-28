namespace TRX_Installer;

public interface IInstallerProgress
{
    void SetDescription(string description);
    void SetSubDescription(string subDescription);
    void SetOuterProgress(int current, int maximum);
    void SetInnerProgress(int current, int maximum);
    void AppendLog(string message);
}
