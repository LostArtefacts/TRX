using Newtonsoft.Json;
using System;
using System.Globalization;

namespace TR1X_ConfigTool.Utils;

public class NumericConverter : JsonConverter<decimal>
{
    public override decimal ReadJson(JsonReader reader, Type objectType, decimal existingValue, bool hasExistingValue, JsonSerializer serializer)
    {
        throw new NotImplementedException();
    }

    public override void WriteJson(JsonWriter writer, decimal value, JsonSerializer serializer)
    {
        writer.WriteRawValue(value.ToString(CultureInfo.InvariantCulture));
    }
}
