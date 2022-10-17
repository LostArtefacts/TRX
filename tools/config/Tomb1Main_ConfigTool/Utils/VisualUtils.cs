using System;
using System.Windows;
using System.Windows.Media;

namespace Tomb1Main_ConfigTool.Utils;

public static class VisualUtils
{
    public static Visual GetChild(DependencyObject root, Type type)
    {
        for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
        {
            Visual child = (Visual)VisualTreeHelper.GetChild(root, i);

            if (child.GetType() == type)
            {
                return child;
            }

            Visual grandChild = GetChild(child, type);
            if (grandChild != null)
            {
                return grandChild;
            }
        }

        return null;
    }
}