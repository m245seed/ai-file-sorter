namespace AiFileSorter.Models;

public class CategorizedFile
{
    public string FilePath { get; set; } = string.Empty;
    public string FileName { get; set; } = string.Empty;
    public FileType Type { get; set; }
    public string Category { get; set; } = string.Empty;
    public string Subcategory { get; set; } = string.Empty;
    public int TaxonomyId { get; set; }
}
