using System.Collections.Generic;

namespace Tomb1Main_ConfigTool.Models;

public class CategoryViewModel
{
    private static readonly string _iconPathFormat = "pack://application:,,,/Tomb1Main_ConfigTool;component/Resources/{0}";
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
        get => string.Format(_iconPathFormat, _category.Image ?? _defaultImage);
    }

    public IEnumerable<BaseProperty> ItemsSource
    {
        get => _category.Properties;
    }

    public double ViewPosition { get; set; }
}