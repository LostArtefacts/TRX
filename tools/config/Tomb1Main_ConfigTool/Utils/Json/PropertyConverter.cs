using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using Tomb1Main_ConfigTool.Models;

namespace Tomb1Main_ConfigTool.Utils;

public class PropertyConverter : JsonConverter
{
    private static readonly JsonSerializerSettings _resolver = new()
    {
        ContractResolver = new PropertyResolver()
    };

    private const string _dataTypeProperty = "DataType";

    public override bool CanConvert(Type typeToConvert)
    {
        return typeof(BaseProperty).IsAssignableFrom(typeToConvert);
    }

    public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
    {
        JObject jo = JObject.Load(reader);
        if (!jo.ContainsKey(_dataTypeProperty))
        {
            throw new JsonException();
        }

        return Enum.Parse<DataType>(jo[_dataTypeProperty].ToString()) switch
        {
            DataType.Bool => JsonConvert.DeserializeObject<BoolProperty>(jo.ToString(), _resolver),
            DataType.Enum => JsonConvert.DeserializeObject<EnumProperty>(jo.ToString(), _resolver),
            DataType.Numeric => JsonConvert.DeserializeObject<NumericProperty>(jo.ToString(), _resolver),
            _ => throw new JsonException(),
        };
    }

    public override bool CanWrite => false;

    public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer) { }
}