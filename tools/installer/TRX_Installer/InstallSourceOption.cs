namespace TRX_Installer;

public class InstallSourceOption
{
    public InstallSourceOption(
        string sourceName,
        IEnumerable<string> directoriesToTry,
        IInstallSource source,
        Func<string, bool> isGameFound
    )
    {
        SourceName = sourceName;
        DirectoriesToTry = directoriesToTry;
        Source = source;
        _isGameFound = isGameFound;
    }

    public IEnumerable<string> DirectoriesToTry { get; }
    public string SourceName { get; }
    public IInstallSource Source { get; }

    private readonly Func<string, bool> _isGameFound;

    public bool IsGameFound(string sourceDirectory)
        => _isGameFound(sourceDirectory);
}
