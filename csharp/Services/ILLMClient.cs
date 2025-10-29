using AiFileSorter.Models;
using System.Threading.Tasks;

namespace AiFileSorter.Services;

public interface ILLMClient
{
    Task<(string category, string subcategory)> CategorizeFileAsync(
        string fileName, 
        FileType fileType, 
        string? filePath = null, 
        bool useSubcategories = true);
}
