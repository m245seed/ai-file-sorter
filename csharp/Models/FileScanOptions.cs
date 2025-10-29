using System;

namespace AiFileSorter.Models;

[Flags]
public enum FileScanOptions
{
    None = 0,
    Files = 1 << 0,         // 0001
    Directories = 1 << 1,   // 0010
    HiddenFiles = 1 << 2    // 0100
}
