using System.Globalization;
using System.Windows;
using System.Windows.Controls;

namespace TR1X_ConfigTool.Utils;

public class NumericValidationRule : ValidationRule
{
    public NumericValidation ValidationRule { get; set; }

    public override ValidationResult Validate(object value, CultureInfo cultureInfo)
    {
        if (!decimal.TryParse(value.ToString(), out decimal val))
        {
            return new ValidationResult(false, ValidationRule.InvalidNumberMessage);
        }

        if (val < ValidationRule.MinValue || val > ValidationRule.MaxValue)
        {
            return new ValidationResult(false, string.Format(ValidationRule.ComparisonFailedMessage, ValidationRule.MinValue, ValidationRule.MaxValue));
        }

        return ValidationResult.ValidResult;
    }
}

public class NumericValidation : DependencyObject
{
    public static readonly DependencyProperty MinValueProperty = DependencyProperty.RegisterAttached
    (
        nameof(MinValue), typeof(decimal), typeof(NumericValidation), new PropertyMetadata(0M)
    );

    public static readonly DependencyProperty MaxValueProperty = DependencyProperty.RegisterAttached
    (
        nameof(MaxValue), typeof(decimal), typeof(NumericValidation), new PropertyMetadata(0M)
    );

    public static readonly DependencyProperty InvalidNumberMessageProperty = DependencyProperty.RegisterAttached
    (
        nameof(InvalidNumberMessage), typeof(string), typeof(NumericValidation), new PropertyMetadata(string.Empty)
    );

    public static readonly DependencyProperty ComparisonFailedMessageProperty = DependencyProperty.RegisterAttached
    (
        nameof(ComparisonFailedMessage), typeof(string), typeof(NumericValidation), new PropertyMetadata(string.Empty)
    );

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

    public string InvalidNumberMessage
    {
        get => (string)GetValue(InvalidNumberMessageProperty);
        set => SetValue(MaxValueProperty, value);
    }

    public string ComparisonFailedMessage
    {
        get => (string)GetValue(ComparisonFailedMessageProperty);
        set => SetValue(ComparisonFailedMessageProperty, value);
    }
}