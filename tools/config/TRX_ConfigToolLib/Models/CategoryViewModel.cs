using TRX_ConfigToolLib.Models.Specification;
using TRX_ConfigToolLib.Utils;

namespace TRX_ConfigToolLib.Models;

public class CategoryViewModel
{
    private static readonly string _defaultImage = "Graphics/graphic1.jpg";

    private readonly Category _category;

    public CategoryViewModel(Category category)
    {
        _category = category;
        ItemsSource = new(category.Properties);
    }

    public string Title
    {
        get => _category.Title;
    }

    public string ImageSource
    {
        get => AssemblyUtils.GetEmbeddedResourcePath(_category.Image ?? _defaultImage);
    }

    public FastObservableCollection<BaseProperty> ItemsSource { get; private set; }

    public double ViewPosition { get; set; }
}
