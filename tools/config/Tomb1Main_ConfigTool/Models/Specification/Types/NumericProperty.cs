using System;

namespace Tomb1Main_ConfigTool.Models;

public class NumericProperty : BaseProperty
{
    private decimal _value;

    public decimal Value
    {
        get => _value;
        set
        {
            decimal clampedValue = Clamp(value);
            if (_value != clampedValue)
            {
                _value = clampedValue;
                NotifyPropertyChanged();
            }
        }
    }

    public decimal DefaultValue { get; set; }

    public override bool IsDefault
    {
        get => Value == DefaultValue;
    }

    public decimal MinimumValue { get; set; } = decimal.MinValue;
    public decimal MaximumValue { get; set; } = decimal.MaxValue;
    public int DecimalPlaces { get; set; }

    public override object ExportValue()
    {
        return Value;
    }

    public override void LoadValue(string value)
    {
        if (decimal.TryParse(value, out decimal d))
        {
            Value = d;
        }
    }

    public override void SetToDefault()
    {
        Value = DefaultValue;
    }

    private decimal Clamp(decimal value)
    {
        return Math.Round(Math.Min(MaximumValue, Math.Max(MinimumValue, value)), DecimalPlaces);
    }
}