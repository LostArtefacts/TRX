using System.IO;

namespace TRX_Installer;

public static class InstallMappings
{
    private sealed record MappingRule(string SourcePattern, string TargetPattern);

    private static readonly Dictionary<string, List<MappingRule>> Rules = new()
    {
        ["tr1"] =
        [
            new(@"data/*.phd", @"games\tr1\levels\*.phd"),
            new(@"data/*.pcx", @"games\tr1\images\*.pcx"),
            new(@"fmv/*", @"games\tr1\fmv\*"),
            new(@"music/*", @"games\tr1\music\*"),
        ],
        ["tr1-ub"] =
        [
            new(@"data/*.phd", @"games\tr1-ub\levels\*.phd"),
            new(@"data/*.pcx", @"games\tr1-ub\images\*.pcx"),
        ],
        ["tr1-demo-pc"] =
        [
            new(@"games/tr1-demo-pc/*", @"games\tr1-demo-pc\*"),
        ],
        ["tr2"] =
        [
            new(@"data/main.sfx", @"games\tr2\main.sfx"),
            new(@"data/*.tr2", @"games\tr2\levels\*.tr2"),
            new(@"data/*.pcx", @"games\tr2\images\*.pcx"),
            new(@"fmv/*", @"games\tr2\fmv\*"),
            new(@"music/*", @"games\tr2\music\*"),
        ],
        ["tr2-gm"] =
        [
            new(@"data/main_gm.sfx", @"games\tr2-gm\main_gm.sfx"),
            new(@"data/*.tr2", @"games\tr2-gm\levels\*.tr2"),
            new(@"data/*.pcx", @"games\tr2-gm\images\*.pcx"),
        ],
        ["tr3"] =
        [
            new(@"data/main.sfx", @"games\tr3\main.sfx"),
            new(@"data/antarc.tr2", @"games\tr3\levels\antarc.tr2"),
            new(@"data/area51.tr2", @"games\tr3\levels\area51.tr2"),
            new(@"data/chamber.tr2", @"games\tr3\levels\chamber.tr2"),
            new(@"data/city.tr2", @"games\tr3\levels\city.tr2"),
            new(@"data/compound.tr2", @"games\tr3\levels\compound.tr2"),
            new(@"data/crash.tr2", @"games\tr3\levels\crash.tr2"),
            new(@"data/house.tr2", @"games\tr3\levels\house.tr2"),
            new(@"data/jungle.tr2", @"games\tr3\levels\jungle.tr2"),
            new(@"data/mines.tr2", @"games\tr3\levels\mines.tr2"),
            new(@"data/nevada.tr2", @"games\tr3\levels\nevada.tr2"),
            new(@"data/office.tr2", @"games\tr3\levels\office.tr2"),
            new(@"data/quadchas.tr2", @"games\tr3\levels\quadchas.tr2"),
            new(@"data/rapids.tr2", @"games\tr3\levels\rapids.tr2"),
            new(@"data/roofs.tr2", @"games\tr3\levels\roofs.tr2"),
            new(@"data/sewer.tr2", @"games\tr3\levels\sewer.tr2"),
            new(@"data/shore.tr2", @"games\tr3\levels\shore.tr2"),
            new(@"data/stpaul.tr2", @"games\tr3\levels\stpaul.tr2"),
            new(@"data/temple.tr2", @"games\tr3\levels\temple.tr2"),
            new(@"data/title.tr2", @"games\tr3\levels\title.tr2"),
            new(@"data/tonyboss.tr2", @"games\tr3\levels\tonyboss.tr2"),
            new(@"data/tower.tr2", @"games\tr3\levels\tower.tr2"),
            new(@"data/triboss.tr2", @"games\tr3\levels\triboss.tr2"),
            new(@"fmv/*", @"games\tr3\fmv\*"),
            new(@"audio/*", @"games\tr3\audio\*"),
            new(@"cuts/*", @"games\tr3\cuts\*"),
            new(@"pix/*", @"games\tr3\images\*"),
        ],
        ["tr3-la"] =
        [
            new(@"data/chunnel.tr2", @"games\tr3-la\levels\chunnel.tr2"),
            new(@"data/scotland.tr2", @"games\tr3-la\levels\scotland.tr2"),
            new(@"data/slinc.tr2", @"games\tr3-la\levels\slinc.tr2"),
            new(@"data/undersea.tr2", @"games\tr3-la\levels\undersea.tr2"),
            new(@"data/willsden.tr2", @"games\tr3-la\levels\willsden.tr2"),
            new(@"data/zoo.tr2", @"games\tr3-la\levels\zoo.tr2"),
            new(@"data/title.tr2", @"games\tr3-la\levels\title.tr2"),
            new(@"data/title_la.tr2", @"games\tr3-la\levels\title.tr2"),
            new(@"data/main.sfx", @"games\tr3-la\main.sfx"),
            new(@"data/main_la.sfx", @"games\tr3-la\main.sfx"),
            new(@"pix/*", @"games\tr3-la\images\*"),
        ],
    };

    public static string Normalize(string relativePath)
    {
        return relativePath.Replace('/', Path.DirectorySeparatorChar)
            .Replace('\\', Path.DirectorySeparatorChar)
            .ToLowerInvariant();
    }

    private static bool TryMatchPattern(
        string path,
        string pattern,
        out string wildcardValue)
    {
        wildcardValue = string.Empty;

        int wildcardPos = pattern.IndexOf('*');
        if (wildcardPos == -1)
        {
            return string.Equals(path, pattern, StringComparison.OrdinalIgnoreCase);
        }

        string prefix = pattern[..wildcardPos];
        string suffix = pattern[(wildcardPos + 1)..];
        if (!path.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)
            || !path.EndsWith(suffix, StringComparison.OrdinalIgnoreCase)
            || path.Length < prefix.Length + suffix.Length)
        {
            return false;
        }

        wildcardValue = path[prefix.Length..(path.Length - suffix.Length)];
        if (wildcardValue.Contains(Path.DirectorySeparatorChar))
        {
            wildcardValue = string.Empty;
            return false;
        }

        return true;
    }

    private static string ApplyPattern(string pattern, string wildcardValue)
    {
        int wildcardPos = pattern.IndexOf('*');
        if (wildcardPos == -1)
        {
            return pattern;
        }

        return pattern[..wildcardPos] + wildcardValue + pattern[(wildcardPos + 1)..];
    }

    private static bool IsShadowedTR3LAAlias(
        string normalizedPath,
        ISet<string> availablePaths)
    {
        if (normalizedPath == Normalize(@"data/title.tr2")
            && availablePaths.Contains(Normalize(@"data/title_la.tr2")))
        {
            return true;
        }

        if (normalizedPath == Normalize(@"data/main.sfx")
            && availablePaths.Contains(Normalize(@"data/main_la.sfx")))
        {
            return true;
        }

        return false;
    }

    public static string? MapOriginalFile(
        string gameId,
        string relativePath,
        ISet<string> availablePaths)
    {
        string normalized = Normalize(relativePath);
        if (gameId == "tr3-la" && IsShadowedTR3LAAlias(normalized, availablePaths))
        {
            return null;
        }

        if (!Rules.TryGetValue(gameId, out List<MappingRule>? rules))
        {
            return null;
        }

        foreach (MappingRule rule in rules)
        {
            if (TryMatchPattern(normalized, Normalize(rule.SourcePattern), out string wildcardValue))
            {
                return ApplyPattern(Normalize(rule.TargetPattern), wildcardValue);
            }
        }

        return null;
    }

    public static string? MapCombinedFile(string gameId, string relativePath)
    {
        string normalized = Normalize(relativePath);
        if (!Rules.TryGetValue(gameId, out List<MappingRule>? rules))
        {
            return null;
        }

        foreach (MappingRule rule in rules)
        {
            if (TryMatchPattern(normalized, Normalize(rule.TargetPattern), out _))
            {
                return normalized;
            }
        }

        return null;
    }
}
