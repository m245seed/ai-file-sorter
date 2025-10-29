using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using AiFileSorter.Models;
using AiFileSorter.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace AiFileSorter.ViewModels;

public partial class MainWindowViewModel : ViewModelBase
{
    private readonly Settings _settings;
    private readonly DatabaseManager _dbManager;
    private readonly FileScanner _fileScanner;
    private ILLMClient? _llmClient;

    [ObservableProperty]
    private string _selectedDirectory = string.Empty;

    [ObservableProperty]
    private bool _useSubcategories = true;

    [ObservableProperty]
    private bool _categorizeFiles = true;

    [ObservableProperty]
    private bool _categorizeDirectories = false;

    [ObservableProperty]
    private bool _isAnalyzing = false;

    [ObservableProperty]
    private string _statusMessage = "Ready";

    [ObservableProperty]
    private ObservableCollection<CategorizedFile> _categorizedFiles = new();

    public MainWindowViewModel()
    {
        Services.Logger.Setup();
        _settings = new Settings();
        _settings.Load();
        _dbManager = new DatabaseManager(_settings.GetConfigDirectory());
        _fileScanner = new FileScanner();

        UseSubcategories = _settings.UseSubcategories;
        CategorizeFiles = _settings.CategorizeFiles;
        CategorizeDirectories = _settings.CategorizeDirectories;

        if (!string.IsNullOrEmpty(_settings.LastUsedDirectory))
        {
            SelectedDirectory = _settings.LastUsedDirectory;
        }

        InitializeLLMClient();
    }

    private void InitializeLLMClient()
    {
        // For demo purposes, using a simple placeholder
        // In production, this would check settings and initialize appropriate client
        if (_settings.LlmChoice == LLMChoice.Remote)
        {
            // Would need API key from secure storage
            _llmClient = null; // Placeholder
        }
        else if (_settings.LlmChoice == LLMChoice.Local_3b || _settings.LlmChoice == LLMChoice.Local_7b)
        {
            if (!string.IsNullOrEmpty(_settings.LocalModelPath))
            {
                _llmClient = new LocalLLMClient(_settings.LocalModelPath);
            }
        }
    }

    [RelayCommand]
    private async Task BrowseDirectory()
    {
        try
        {
            var dialog = new Avalonia.Controls.OpenFolderDialog
            {
                Title = "Select Directory to Analyze"
            };

            // Note: This requires access to the Window, which would need to be passed in
            // For now, this is a placeholder that sets a demo path
            // In a real implementation, you'd get the window reference from the view
            StatusMessage = "Directory browser not yet fully implemented - please type path manually";
            
            // Example of how it would work with proper window reference:
            // var result = await dialog.ShowAsync(window);
            // if (!string.IsNullOrEmpty(result))
            // {
            //     SelectedDirectory = result;
            // }
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error: {ex.Message}";
        }
    }

    [RelayCommand]
    private async Task AnalyzeDirectory()
    {
        if (string.IsNullOrEmpty(SelectedDirectory) || !Directory.Exists(SelectedDirectory))
        {
            StatusMessage = "Please select a valid directory";
            return;
        }

        if (_llmClient == null)
        {
            // Use rule-based local client as fallback
            _llmClient = new LocalLLMClient(string.Empty);
        }

        IsAnalyzing = true;
        StatusMessage = "Analyzing directory...";
        CategorizedFiles.Clear();

        try
        {
            var options = FileScanOptions.None;
            if (CategorizeFiles) options |= FileScanOptions.Files;
            if (CategorizeDirectories) options |= FileScanOptions.Directories;

            var entries = _fileScanner.GetDirectoryEntries(SelectedDirectory, options);
            StatusMessage = $"Found {entries.Count} items to categorize";

            var categorizedList = new List<CategorizedFile>();

            for (int i = 0; i < entries.Count; i++)
            {
                var entry = entries[i];
                StatusMessage = $"Categorizing {i + 1}/{entries.Count}: {entry.FileName}";

                // Check if already categorized in database
                var existing = _dbManager.GetCategorization(entry.FullPath);
                if (existing != null && File.Exists(entry.FullPath))
                {
                    categorizedList.Add(existing);
                    continue;
                }

                // Categorize using LLM
                var (category, subcategory) = await _llmClient.CategorizeFileAsync(
                    entry.FileName, 
                    entry.Type, 
                    entry.FullPath, 
                    UseSubcategories);

                var categorized = new CategorizedFile
                {
                    FilePath = entry.FullPath,
                    FileName = entry.FileName,
                    Type = entry.Type,
                    Category = category,
                    Subcategory = subcategory
                };

                categorizedList.Add(categorized);
                _dbManager.SaveCategorization(categorized);
            }

            foreach (var item in categorizedList)
            {
                CategorizedFiles.Add(item);
            }

            StatusMessage = $"Categorized {categorizedList.Count} items";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error: {ex.Message}";
        }
        finally
        {
            IsAnalyzing = false;
        }
    }

    [RelayCommand]
    private async Task SortFiles()
    {
        if (CategorizedFiles.Count == 0)
        {
            StatusMessage = "No files to sort";
            return;
        }

        StatusMessage = "Sorting files...";
        int movedCount = 0;

        try
        {
            foreach (var file in CategorizedFiles)
            {
                var targetDir = Path.Combine(SelectedDirectory, file.Category);
                if (!string.IsNullOrEmpty(file.Subcategory))
                {
                    targetDir = Path.Combine(targetDir, file.Subcategory);
                }

                Directory.CreateDirectory(targetDir);

                var targetPath = Path.Combine(targetDir, file.FileName);
                
                try
                {
                    if (file.Type == FileType.File)
                    {
                        File.Move(file.FilePath, targetPath, true);
                    }
                    else if (file.Type == FileType.Directory)
                    {
                        Directory.Move(file.FilePath, targetPath);
                    }
                    movedCount++;
                }
                catch (Exception ex)
                {
                    Services.Logger.GetLogger("core_logger").Error(ex, 
                        "Failed to move {FilePath}", file.FilePath);
                }
            }

            StatusMessage = $"Sorted {movedCount} items";
            CategorizedFiles.Clear();
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error sorting files: {ex.Message}";
        }
    }

    partial void OnUseSubcategoriesChanged(bool value)
    {
        _settings.UseSubcategories = value;
        _settings.Save();
    }

    partial void OnCategorizeFilesChanged(bool value)
    {
        _settings.CategorizeFiles = value;
        _settings.Save();
    }

    partial void OnCategorizeDirectoriesChanged(bool value)
    {
        _settings.CategorizeDirectories = value;
        _settings.Save();
    }

    partial void OnSelectedDirectoryChanged(string value)
    {
        _settings.LastUsedDirectory = value;
        _settings.Save();
    }
}

