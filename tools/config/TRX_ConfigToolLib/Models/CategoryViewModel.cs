using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models;

public class CategoryViewModel
{
    private static readonly string _defaultImage = "Graphics/graphic1.jpg";

    private readonly Category _category;

    public CategoryViewModel(Category category)
    {
        _category = category;
    }

    public string Title
    {
        get => _category.Title;
    }

    public string ImageSource
    {
        get => AssemblyUtils.GetEmbeddedResourcePath(_category.Image ?? _defaultImage);
    }

    public IEnumerable<BaseProperty> ItemsSource
    {
        get => _category.Properties;
    }

    public double ViewPosition { get; set; }
}
