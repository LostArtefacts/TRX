using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models;

public class Specification
{
    public Dictionary<string, List<EnumOption>> Enums { get; private set; }
    public List<Category> CategorisedProperties { get; private set; }
    public List<BaseProperty> Properties { get; private set; }

    public Specification(string sourceData)
    {
        JObject data = JObject.Parse(sourceData);
        JObject enumData = data.ContainsKey(nameof(Enums))
            ? data[nameof(Enums)].ToObject<JObject>()
            : new();
        Enums = new();

        foreach (var (key, value) in enumData)
        {
            List<string> enumValues = value.ToObject<List<string>>();
            Enums[key] = enumValues.Select(val => new EnumOption
            {
                EnumName = key,
                ID = val
            }).ToList();
        }

        string categoryData = data[nameof(CategorisedProperties)].ToString();
        PropertyConverter converter = new();
        CategorisedProperties = JsonConvert.DeserializeObject<List<Category>>(categoryData, converter);
        Properties = new();

        foreach (BaseProperty property in CategorisedProperties.SelectMany(c => c.Properties))
        {
            property.Initialise(this);
            Properties.Add(property);
        }
    }
}
