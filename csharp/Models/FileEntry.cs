namespace AiFileSorter.Models;

public class FileEntry
{
    public string FullPath { get; set; } = string.Empty;
    public string FileName { get; set; } = string.Empty;
    public FileType Type { get; set; }
}
