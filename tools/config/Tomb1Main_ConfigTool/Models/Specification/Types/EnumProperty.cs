using System.Collections.Generic;

namespace Tomb1Main_ConfigTool.Models;

public class EnumProperty : BaseProperty
{
    public string EnumKey { get; set; }
    
    private EnumOption _value;

    public EnumOption Value
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

    public string DefaultValue { get; set; }

    public override bool IsDefault
    {
        get => Value.ID == DefaultValue;
    }

    public List<EnumOption> Options { get; set; }

    public override object ExportValue()
    {
        return Value.ID;
    }

    public override void LoadValue(string value)
    {
        Value = Options.Find(o => o.ID == value);
    }

    public override void SetToDefault()
    {
        LoadValue(DefaultValue);
    }

    public override void Initialise(Specification specification)
    {
        if (specification.Enums.ContainsKey(EnumKey))
        {
            Options = specification.Enums[EnumKey];
        }
        base.Initialise(specification);
    }
}