using TRX_ConfigToolLib.Models.Lang;

namespace TRX_ConfigToolLib.Models.Specification;

public class EnumOption
{
    public string EnumName { get; set; }
    public string ID { get; set; }
    public string Title
    {
        get => Language.Instance.Enums[EnumName][ID];
    }
}
