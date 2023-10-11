using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using TR1X_ConfigTool.Utils;

namespace TR1X_ConfigTool.Models;

public class Language
{
    private static readonly string _langPathFormat = "Resources.Lang.{0}.json";
    private static readonly string _defaultCulture = "en-US";

    static Language()
    {
        LoadCulture();
    }

    public static Language Instance { get; private set; }

    public Dictionary<string, string> Controls { get; set; }
    public Dictionary<string, Dictionary<string, string>> Enums { get; set; }
    public Dictionary<string, string> Categories { get; set; }
    public Dictionary<string, PropertyText> Properties { get; set; }

    private static void LoadCulture()
    {
        CultureInfo defaultCulture = CultureInfo.GetCultureInfo(_defaultCulture);
        JObject defaultData = ReadLanguage(defaultCulture.TwoLetterISOLanguageName);

        if (CultureInfo.CurrentCulture != defaultCulture)
        {
            // Merge the main language first if it exists, and then the country specific if that exists.
            // e.g. fr.json would load first, then fr-BE.json.
            MergeLanguage(defaultData, CultureInfo.CurrentCulture.TwoLetterISOLanguageName);
            MergeLanguage(defaultData, CultureInfo.CurrentCulture.Name);
        }

        Instance = JsonConvert.DeserializeObject<Language>(defaultData.ToString());
    }

    private static JObject ReadLanguage(string tag)
    {
        string langPath = string.Format(_langPathFormat, tag);
        if (AssemblyUtils.ResourceExists(langPath))
        {
            using Stream stream = AssemblyUtils.GetResourceStream(langPath);
            using StreamReader reader = new(stream);
            return JObject.Parse(reader.ReadToEnd());
        }

        return null;
    }

    private static void MergeLanguage(JObject data, string tag)
    {
        JObject cultureData = ReadLanguage(tag);
        if (cultureData != null)
        {
            data.Merge(cultureData);
        }
    }
}
