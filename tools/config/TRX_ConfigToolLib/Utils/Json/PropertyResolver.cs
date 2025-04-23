using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using TRX_ConfigToolLib.Models.Specification;

namespace TRX_ConfigToolLib.Utils.Json;

public class PropertyResolver : DefaultContractResolver
{
    protected override JsonConverter ResolveContractConverter(Type objectType)
    {
        return typeof(BaseProperty).IsAssignableFrom(objectType)
            ? null
            : base.ResolveContractConverter(objectType);
    }
}
