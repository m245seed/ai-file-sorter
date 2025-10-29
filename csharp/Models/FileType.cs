namespace AiFileSorter.Models;

public enum FileType
{
    File,
    Directory
}

public static class FileTypeExtensions
{
    public static string ToDisplayString(this FileType type)
    {
        return type switch
        {
            FileType.File => "File",
            FileType.Directory => "Directory",
            _ => "Unknown"
        };
    }
}
