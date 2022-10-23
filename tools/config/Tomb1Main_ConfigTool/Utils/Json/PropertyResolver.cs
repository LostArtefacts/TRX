using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System;
using Tomb1Main_ConfigTool.Models;

namespace Tomb1Main_ConfigTool.Utils;

public class PropertyResolver : DefaultContractResolver
{
    protected override JsonConverter ResolveContractConverter(Type objectType)
    {
        return typeof(BaseProperty).IsAssignableFrom(objectType)
            ? null
            : base.ResolveContractConverter(objectType);
    }
}