using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using TR1X_ConfigTool.Utils;

namespace TR1X_ConfigTool.Controls;

public partial class NumericUpDown : UserControl
{
    public static readonly DependencyProperty ValueProperty = DependencyProperty.Register
    (
        nameof(Value), typeof(decimal), typeof(NumericUpDown), new PropertyMetadata(decimal.MinValue, (sender, args) =>
        {
            if (sender is NumericUpDown numericUpDown)
            {
                numericUpDown.OnValueChanged();
            }
        })
    );

    public static readonly DependencyProperty TextProperty = DependencyProperty.Register
    (
        nameof(Text), typeof(string), typeof(NumericUpDown), new PropertyMetadata(string.Empty)
    );

    public static readonly DependencyProperty DecimalPlacesProperty = DependencyProperty.Register
    (
        nameof(DecimalPlaces), typeof(int), typeof(NumericUpDown), new PropertyMetadata(0, (sender, args) =>
        {
            if (sender is NumericUpDown numericUpDown)
            {
                numericUpDown.UpdateDisplayText();
            }
        })
    );

    public static readonly DependencyProperty MinValueProperty = DependencyProperty.Register
    (
        nameof(MinValue), typeof(decimal), typeof(NumericUpDown), new PropertyMetadata(decimal.MinValue)
    );

    public static readonly DependencyProperty MaxValueProperty = DependencyProperty.Register
    (
        nameof(MaxValue), typeof(decimal), typeof(NumericUpDown), new PropertyMetadata(decimal.MaxValue)
    );

    public event EventHandler ValueChanged;

    private bool _directValueSet;

    private void OnValueChanged()
    {
        if (!_directValueSet)
        {
            UpdateDisplayText();
        }
    }

    public decimal Value
    {
        get => (decimal)GetValue(ValueProperty);
        set
        {
            _directValueSet = true;
            SetValue(ValueProperty, Clamp(value));
            SetDisplayText();
            ValueChanged?.Invoke(this, EventArgs.Empty);
            _directValueSet = false;
        }
    }

    private bool _updateDisplayText;
    public string Text
    {
        get => (string)GetValue(TextProperty);
        set => SetValue(TextProperty, value);
    }

    public int DecimalPlaces
    {
        get => (int)GetValue(DecimalPlacesProperty);
        set
        {
            SetValue(DecimalPlacesProperty, Math.Max(0, value));
            _stepSize = -1;
        }
    }

    private decimal _stepSize = -1;
    public decimal StepSize
    {
        get
        {
            if (_stepSize == -1)
            {
                _stepSize = (decimal)Math.Pow(10, DecimalPlaces * -1);
            }
            return _stepSize;
        }
    }

    public decimal MinValue
    {
        get => (decimal)GetValue(MinValueProperty);
        set => SetValue(MinValueProperty, value);
    }

    public decimal MaxValue
    {
        get => (decimal)GetValue(MaxValueProperty);
        set => SetValue(MaxValueProperty, value);
    }

    public NumericUpDown()
    {
        InitializeComponent();
        _textBox.DataContext = this;
    }

    private RelayCommand _spinUpCommand;
    public ICommand SpinUpCommand
    {
        get => _spinUpCommand ??= new RelayCommand(SpinUp, CanSpinUp);
    }

    private void SpinUp()
    {
        AmendValue(StepSize);
    }

    private bool CanSpinUp()
    {
        return Value < MaxValue;
    }

    private RelayCommand _spinDownCommand;
    public ICommand SpinDownCommand
    {
        get => _spinDownCommand ??= new RelayCommand(SpinDown, CanSpinDown);
    }

    private void SpinDown()
    {
        AmendValue(-StepSize);
    }

    private bool CanSpinDown()
    {
        return Value > MinValue;
    }

    private void AmendValue(decimal adjustment)
    {
        _updateDisplayText = true;
        Value += adjustment;
        _updateDisplayText = false;
    }

    private decimal Clamp(decimal value)
    {
        value = Math.Round(value, DecimalPlaces);
        return Math.Min(MaxValue, Math.Max(MinValue, value));
    }

    private void TextBox_Pasting(object sender, DataObjectPastingEventArgs e)
    {
        object data = e.DataObject.GetData(DataFormats.UnicodeText);
        if (!IsDataClean(data.ToString()))
        {
            e.CancelCommand();
        }
    }

    private void TextBox_TextInput(object sender, TextCompositionEventArgs e)
    {
        e.Handled = !IsDataClean(e.Text);
    }

    private static bool IsDataClean(string data)
    {
        return decimal.TryParse(data, out decimal _)
            || data == CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator
            || data == CultureInfo.CurrentCulture.NumberFormat.NumberGroupSeparator
            || data == CultureInfo.CurrentCulture.NumberFormat.NegativeSign;
    }

    private void TextBox_PreviewKeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Up || e.Key == Key.Down)
        {
            decimal step = StepSize;
            if (Keyboard.IsKeyDown(Key.RightCtrl) || Keyboard.IsKeyDown(Key.LeftCtrl))
            {
                step *= 5;
            }
            if (e.Key == Key.Down)
            {
                step *= -1;
            }

            AmendValue(step);
        }
        else if (e.Key == Key.Enter)
        {
            UpdateDisplayText();
        }
    }

    private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        if (!_updateDisplayText
            && sender is TextBox textBox
            && decimal.TryParse(textBox.Text, out decimal val)
            && val >= MinValue && val <= MaxValue)
        {
            Value = val;
        }
    }

    private void TextBox_LostFocus(object sender, RoutedEventArgs e)
    {
        UpdateDisplayText();
    }

    private void UpdateDisplayText()
    {
        _updateDisplayText = true;
        SetDisplayText();
        _updateDisplayText = false;
    }

    private void SetDisplayText()
    {
        if (_updateDisplayText)
        {
            Text = Value.ToString("F" + DecimalPlaces.ToString(CultureInfo.CurrentCulture), CultureInfo.CurrentCulture);
        }
    }
}
