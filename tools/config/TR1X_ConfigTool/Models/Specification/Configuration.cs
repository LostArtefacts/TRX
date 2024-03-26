using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.IO;
using TR1X_ConfigTool.Utils;

namespace TR1X_ConfigTool.Models;

public class Configuration
{
    private static readonly string _specificationPath = "Resources.specification.json";
    private static readonly JsonSerializerSettings _serializerSettings = new()
    {
        Converters = new NumericConverter[] { new NumericConverter() },
        Formatting = Formatting.Indented
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
        using Stream stream = AssemblyUtils.GetResourceStream(_specificationPath);
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

            foreach (KeyValuePair<string, JToken> pair in convertedData)
            {
                if (!_activeData.ContainsKey(pair.Key) || !_activeData[pair.Key].Equals(pair.Value))
                {
                    return true;
                }
            }
        }

        return false;
    }

    public bool IsDataDefault()
    {
        foreach (BaseProperty property in Properties)
        {
            if (!property.IsDefault)
            {
                return false;
            }
        }

        return true;
    }

    public void Read(string jsonPath)
    {
        JObject externalData = File.Exists(jsonPath)
            ? JObject.Parse(File.ReadAllText(jsonPath))
            : new();
        JObject activeData = new();

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
