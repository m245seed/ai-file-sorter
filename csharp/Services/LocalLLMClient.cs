using AiFileSorter.Models;
using System;
using System.IO;
using System.Threading.Tasks;

namespace AiFileSorter.Services;

public class LocalLLMClient : ILLMClient
{
    private readonly Serilog.ILogger _logger;
    private readonly string _modelPath;

    public LocalLLMClient(string modelPath)
    {
        _modelPath = modelPath;
        _logger = Logger.GetLogger("llm_logger");
    }

    public async Task<(string category, string subcategory)> CategorizeFileAsync(
        string fileName, 
        FileType fileType, 
        string? filePath = null, 
        bool useSubcategories = true)
    {
        // Note: This is a placeholder implementation
        // In a full implementation, this would use llama.cpp via P/Invoke or a wrapper library
        // For now, we'll use a simple rule-based categorization
        _logger.Warning("Local LLM not fully implemented, using rule-based categorization");

        return await Task.Run(() => CategorizeByRules(fileName, fileType));
    }

    private (string category, string subcategory) CategorizeByRules(string fileName, FileType fileType)
    {
        var extension = Path.GetExtension(fileName).ToLowerInvariant();
        
        // Simple rule-based categorization
        var category = extension switch
        {
            ".jpg" or ".jpeg" or ".png" or ".gif" or ".bmp" or ".svg" or ".webp" => "Images",
            ".mp4" or ".avi" or ".mkv" or ".mov" or ".wmv" or ".flv" => "Videos",
            ".mp3" or ".wav" or ".flac" or ".aac" or ".ogg" or ".m4a" => "Music",
            ".pdf" or ".doc" or ".docx" or ".txt" or ".rtf" or ".odt" => "Documents",
            ".xls" or ".xlsx" or ".csv" or ".ods" => "Spreadsheets",
            ".ppt" or ".pptx" or ".odp" => "Presentations",
            ".zip" or ".rar" or ".7z" or ".tar" or ".gz" => "Archives",
            ".exe" or ".msi" or ".dmg" or ".app" or ".deb" or ".rpm" => "Applications",
            ".cs" or ".cpp" or ".h" or ".java" or ".py" or ".js" or ".ts" or ".go" => "Source Code",
            _ => fileType == FileType.Directory ? "Folders" : "Other"
        };

        var subcategory = extension switch
        {
            ".jpg" or ".jpeg" => "JPEG Images",
            ".png" => "PNG Images",
            ".mp4" => "MP4 Videos",
            ".mp3" => "MP3 Audio",
            ".pdf" => "PDF Documents",
            ".cs" => "C# Code",
            ".cpp" or ".h" => "C++ Code",
            _ => string.Empty
        };

        return (category, subcategory);
    }
}
