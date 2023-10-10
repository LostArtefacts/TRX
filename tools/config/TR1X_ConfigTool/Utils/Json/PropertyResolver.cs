using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System;
using TR1X_ConfigTool.Models;

namespace TR1X_ConfigTool.Utils;

public class PropertyResolver : DefaultContractResolver
{
    protected override JsonConverter ResolveContractConverter(Type objectType)
    {
        return typeof(BaseProperty).IsAssignableFrom(objectType)
            ? null
            : base.ResolveContractConverter(objectType);
    }
}
