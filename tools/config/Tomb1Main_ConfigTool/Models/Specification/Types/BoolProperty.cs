namespace Tomb1Main_ConfigTool.Models;

public class BoolProperty : BaseProperty
{
    private bool _value;

    public bool Value
    {
        get => _value;
        set
        {
            if (_value != value)
            {
                _value = value;
                NotifyPropertyChanged();
            }
        }
    }

    public bool DefaultValue { get; set; }

    public override bool IsDefault
    {
        get => Value == DefaultValue;
    }

    public override object ExportValue()
    {
        return Value;
    }

    public override void LoadValue(string value)
    {
        if (bool.TryParse(value, out bool val))
        {
            Value = val;
        }
    }

    public override void SetToDefault()
    {
        Value = DefaultValue;
    }
}