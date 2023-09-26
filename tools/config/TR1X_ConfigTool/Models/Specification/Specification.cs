using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Linq;
using TR1X_ConfigTool.Utils;

namespace TR1X_ConfigTool.Models;

public class Specification
{
    public Dictionary<string, List<EnumOption>> Enums { get; private set; }
    public List<Category> CategorisedProperties { get; private set; }
    public List<BaseProperty> Properties { get; private set; }

    public Specification(string sourceData)
    {
        JObject data = JObject.Parse(sourceData);
        JObject enumData = data[nameof(Enums)].ToObject<JObject>();
        Enums = new();

        foreach (KeyValuePair<string, JToken> enumType in enumData)
        {
            List<string> enumValues = enumType.Value.ToObject<List<string>>();
            IEnumerable<EnumOption> options = enumValues.Select(val => new EnumOption
            {
                EnumName = enumType.Key,
                ID = val
            });
            Enums[enumType.Key] = options.ToList();
        }

        string categoryData = data[nameof(CategorisedProperties)].ToString();
        PropertyConverter converter = new();
        CategorisedProperties = JsonConvert.DeserializeObject<List<Category>>(categoryData, converter);
        Properties = new();

        foreach (Category category in CategorisedProperties)
        {
            foreach (BaseProperty property in category.Properties)
            {
                property.Initialise(this);
                Properties.Add(property);
            }
        }
    }
}