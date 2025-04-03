using TRX_ConfigToolLib.Models.Lang;
using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models.Specification;

public abstract class BaseProperty : BaseNotifyPropertyChanged
{
    private static readonly object _nullValue = new();

    public string Field { get; set; }

    public string Title
    {
        get => Language.Instance.Properties[Field].Title;
    }

    public string Description
    {
        get => Language.Instance.Properties[Field].Description;
    }

    public string NormalisedText { get; private set; }

    public abstract object ExportValue();
    public abstract void LoadValue(string value);
    public abstract void SetToDefault();
    public abstract bool IsDefault { get; }

    private object _enforcedValue = _nullValue;
    public object EnforcedValue
    {
        get => _enforcedValue;
        set
        {
            if (_enforcedValue != value)
            {
                _enforcedValue = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged(nameof(IsEnabled));
            }
        }
    }

    public bool IsEnabled => _enforcedValue == _nullValue;

    public virtual void Initialise(Specification specification)
    {
        SetToDefault();

        string text = (Title + " " + Description).ToLower();
        NormalisedText = TextUtilities.Normalise(text);
    }
}
