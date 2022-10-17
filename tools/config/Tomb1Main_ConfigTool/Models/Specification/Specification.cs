using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using Tomb1Main_ConfigTool.Utils;

namespace Tomb1Main_ConfigTool.Models;

public class Specification
{
    public Dictionary<string, List<EnumOption>> Enums { get; private set; }
    public List<Category> CategorisedProperties { get; private set; }
    public List<BaseProperty> Properties { get; private set; }

    public Specification(string sourceData)
    {
        JObject data = JObject.Parse(sourceData);
        Dictionary<string, List<string>> enumTypes = JsonConvert.DeserializeObject<Dictionary<string, List<string>>>(data[nameof(Enums)].ToString());

        Enums = new();
        foreach (string enumName in enumTypes.Keys)
        {
            Enums[enumName] = new();
            foreach (string enumValueID in enumTypes[enumName])
            {
                Enums[enumName].Add(new()
                {
                    EnumName = enumName,
                    ID = enumValueID
                });
            }
        }

        Properties = new();
        CategorisedProperties = JsonConvert.DeserializeObject<List<Category>>(data[nameof(CategorisedProperties)].ToString(), new PropertyConverter());
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