﻿namespace TRX_ConfigToolLib.Models;

public class EnumOption
{
    public string EnumName { get; set; }
    public string ID { get; set; }
    public string Title
    {
        get => Language.Instance.Enums[EnumName][ID];
    }
}
