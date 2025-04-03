using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.IO;
using TRX_ConfigToolLib.Utils;
using TRX_ConfigToolLib.Utils.Json;

namespace TRX_ConfigToolLib.Models.Specification;

public class Configuration
{
    private static readonly string _enforcedKey = "enforced_config";
    private static readonly string _specificationPath = "Resources.specification.json";
    private static readonly JsonSerializerSettings _serializerSettings = new()
    {
        Converters = new NumericConverter[] { new() },
        Formatting = Formatting.Indented,
    };

    private readonly Specification _specification;
    private JObject _activeData, _externalData;

    public IReadOnlyList<Category> Categories
    {
        get => _specification.CategorisedProperties;
    }

    public IReadOnlyList<BaseProperty> Properties
    {
        get => _specification.Properties;
    }

    public Configuration()
    {
        using Stream stream = AssemblyUtils.GetResourceStream(_specificationPath, false);
        using StreamReader reader = new(stream);
        _specification = new Specification(reader.ReadToEnd());
        RestoreDefaults();
    }

    public void RestoreDefaults()
    {
        foreach (BaseProperty property in Properties)
        {
            property.SetToDefault();
        }
    }

    public bool IsDataDirty()
    {
        if (_activeData != null)
        {
            JObject convertedData = GetConvertedData();
            if (convertedData.Count != _activeData.Count)
            {
                return true;
            }

            foreach (var (key, value) in convertedData)
            {
                if (!_activeData.ContainsKey(key) || !_activeData[key].Equals(value))
                {
                    return true;
                }
            }
        }

        return false;
    }

    public bool IsDataDefault()
    {
        return Properties.All(p => p.IsDefault);
    }

    public bool HasReadOnlyItems()
    {
        return Properties.Any(p => !p.IsEnabled);
    }

    public void Read(string jsonPath, string enforcedDataName)
    {
        JObject externalData = File.Exists(jsonPath)
            ? JObject.Parse(File.ReadAllText(jsonPath))
            : new();
        JObject activeData = new();

        string enforcedPath = Path.Combine(Path.GetDirectoryName(jsonPath), enforcedDataName);
        JObject enforcedData = null;
        if (File.Exists(enforcedPath))
        {
            JObject extraData = JObject.Parse(File.ReadAllText(enforcedPath));
            enforcedData = extraData.ContainsKey(_enforcedKey)
                ? extraData[_enforcedKey].ToObject<JObject>()
                : null;
        }

        foreach (BaseProperty property in Properties)
        {
            if (externalData.ContainsKey(property.Field))
            {
                property.LoadValue(externalData[property.Field].ToString());
                externalData.Remove(property.Field);
            }
            else
            {
                property.SetToDefault();
            }

            if (enforcedData != null && enforcedData.ContainsKey(property.Field))
            {
                property.EnforcedValue = enforcedData[property.Field].ToString();
            }
            activeData[property.Field] = JToken.FromObject(property.ExportValue());
        }

        _activeData = activeData;
        _externalData = externalData;
    }

    public void Write(string jsonPath)
    {
        JObject data = GetConvertedData();
        JObject newActiveData = new(data);

        // If the file already exists, re-read any external data from it. Otherwise if writing
        // to a new file, whatever external data was captured in Read will be restored.
        if (File.Exists(jsonPath))
        {
            JObject existingData = GetExistingExternalData(jsonPath);
            if (existingData != null)
            {
                _externalData = existingData;
            }
        }
        data.Merge(_externalData);

        File.WriteAllText(jsonPath, JsonConvert.SerializeObject(data, _serializerSettings));

        _activeData = newActiveData;
    }

    private JObject GetConvertedData()
    {
        JObject data = new();
        foreach (BaseProperty property in Properties)
        {
            data[property.Field] = JToken.FromObject(property.ExportValue());
        }
        return data;
    }

    private JObject GetExistingExternalData(string jsonPath)
    {
        try
        {
            JObject data = JObject.Parse(File.ReadAllText(jsonPath));
            foreach (BaseProperty property in Properties)
            {
                if (data.ContainsKey(property.Field))
                {
                    data.Remove(property.Field);
                }
            }
            return data;
        }
        catch
        {
            return null;
        }
    }
}
