using System.IO;
using System.Text.RegularExpressions;

namespace TRX_Installer;

public class CueFile
{
    private static readonly Dictionary<string, CueFile?> _cache = [];

    public readonly List<CueTrack> TrackList = new();

    // Reads a cue sheet, or returns null when the file is not a usable cue
    // sheet. Use this to probe a file whose format is not known in advance;
    // the constructor reports the reason with an exception instead.
    public static CueFile? TryLoad(string cueFilePath)
    {
        if (!File.Exists(cueFilePath))
        {
            return null;
        }

        // Avoid scanning unchanged files more than once per session.
        var cueKey = GenerateCueKey(cueFilePath);
        if (!_cache.TryGetValue(cueKey, out CueFile? cue))
        {
            cue = TryLoadCue(cueFilePath);
            _cache[cueKey] = cue;
        }

        return cue;
    }

    private static CueFile? TryLoadCue(string cueFilePath)
    {
        try
        {
            CueFile cueFile = new(cueFilePath);
            return cueFile.TrackList.Count > 0 ? cueFile : null;
        }
        catch
        {
            return null;
        }
    }

    private static string GenerateCueKey(string cueFilePath)
    {
        // Less expensive than performing checksums on large files repeatedly.
        return cueFilePath.ToLowerInvariant()
            + File.GetLastWriteTime(cueFilePath);
    }

    public CueFile(string cueFilePath)
    {
        _cueFilePath = cueFilePath;
        string cueFileContent;
        using (TextReader cueReader = new StreamReader(cueFilePath))
        {
            cueFileContent = cueReader.ReadToEnd();
        }

        MatchCollection fileMatches = FileGroupRegex.Matches(cueFileContent);
        if (fileMatches.Count == 0)
        {
            throw new ApplicationException($"Could not parse {cueFilePath}: no tracks were found");
        }

        foreach (Match fileMatch in fileMatches.Cast<Match>())
        {
            string binFilePath = GetBinFilePath(fileMatch.Groups["name"].Value.Trim('"'));
            MatchCollection matches = TrackRegex.Matches(fileMatch.Groups["content"].Value);

            if (matches.Count == 0)
            {
                throw new ApplicationException($"Could not parse {cueFilePath}: no tracks were found");
            }

            CueTrack? track = null;
            CueTrack? prevTrack = null;
            foreach (Match trackMatch in matches.Cast<Match>())
            {
                track = new CueTrack(
                    binFilePath,
                    int.Parse(trackMatch.Groups["track"].Value),
                    trackMatch.Groups["mode"].Value,
                    trackMatch.Groups["time"].Value);

                if (prevTrack is not null)
                {
                    prevTrack.Stop = track.StartPosition - 1;
                    prevTrack.StopSector = track.StartSector;
                }
                TrackList.Add(track);
                prevTrack = track;
            }

            if (track is null)
            {
                return;
            }

            track.Stop = GetBinFileLength(binFilePath);
            track.StopSector = track.Stop / CueTrack.SectorLength;
        }
    }

    private static readonly Regex FileGroupRegex = new(
        @"^file\s+(?<name>""[^""]+""|[^""\s]+)\s+(?<mode>\w+)\s+(?<content>(.(?!^file))*)",
        RegexOptions.IgnoreCase | RegexOptions.Multiline | RegexOptions.Singleline);

    private static readonly Regex TrackRegex = new(
        @"track\s+?(?<track>\d+?)\s+?(?<mode>\S+?)[\s$]+?index\s+?\d+?\s+?(?<time>\S*)",
        RegexOptions.IgnoreCase | RegexOptions.Multiline);

    private readonly string _cueFilePath;

    private static long GetBinFileLength(string binFilePath)
    {
        FileInfo fileInfo = new(binFilePath);
        return fileInfo.Length;
    }

    private string GetBinFilePath(string name)
    {
        string cueDirectory = Path.GetDirectoryName(_cueFilePath)!;
        string result = Path.Combine(cueDirectory, Path.GetFileName(name));
        if (!File.Exists(result))
        {
            result = Path.Combine(cueDirectory, Path.GetFileNameWithoutExtension(_cueFilePath) + ".bin");
        }
        return result;
    }
}
