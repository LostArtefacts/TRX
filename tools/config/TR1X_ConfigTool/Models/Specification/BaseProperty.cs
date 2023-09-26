namespace TR1X_ConfigTool.Models;

public abstract class BaseProperty : BaseNotifyPropertyChanged
{
    public string Field { get; set; }

    public string Title
    {
        get => Language.Instance.Properties[Field].Title;
    }

    public string Description
    {
        get => Language.Instance.Properties[Field].Description;
    }

    public abstract object ExportValue();
    public abstract void LoadValue(string value);
    public abstract void SetToDefault();
    public abstract bool IsDefault { get; }

    public virtual void Initialise(Specification specification)
    {
        SetToDefault();
    }
}