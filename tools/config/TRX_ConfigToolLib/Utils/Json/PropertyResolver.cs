using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using TRX_ConfigToolLib.Models;

namespace TRX_ConfigToolLib.Utils;

public class PropertyResolver : DefaultContractResolver
{
    protected override JsonConverter ResolveContractConverter(Type objectType)
    {
        return typeof(BaseProperty).IsAssignableFrom(objectType)
            ? null
            : base.ResolveContractConverter(objectType);
    }
}
