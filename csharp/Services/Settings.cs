using AiFileSorter.Models;
using Newtonsoft.Json;
using System;
using System.IO;

namespace AiFileSorter.Services;

public class Settings
{
    private readonly string _configDir;
    private readonly string _settingsFile;
    private readonly Serilog.ILogger _logger;

    public LLMChoice LlmChoice { get; set; } = LLMChoice.Unset;
    public string? LocalModelPath { get; set; }
    public bool UseSubcategories { get; set; } = true;
    public bool CategorizeFiles { get; set; } = true;
    public bool CategorizeDirectories { get; set; } = false;
    public string? LastUsedDirectory { get; set; }

    public Settings()
    {
        _logger = Logger.GetLogger("core_logger");
        _configDir = GetConfigDirectory();
        _settingsFile = Path.Combine(_configDir, "settings.json");
        Directory.CreateDirectory(_configDir);
    }

    public string GetConfigDirectory()
    {
        return Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "AiFileSorter"
        );
    }

    public void Load()
    {
        try
        {
            if (File.Exists(_settingsFile))
            {
                var json = File.ReadAllText(_settingsFile);
                var loaded = JsonConvert.DeserializeObject<Settings>(json);
                if (loaded != null)
                {
                    LlmChoice = loaded.LlmChoice;
                    LocalModelPath = loaded.LocalModelPath;
                    UseSubcategories = loaded.UseSubcategories;
                    CategorizeFiles = loaded.CategorizeFiles;
                    CategorizeDirectories = loaded.CategorizeDirectories;
                    LastUsedDirectory = loaded.LastUsedDirectory;
                    _logger.Information("Settings loaded successfully");
                }
            }
            else
            {
                _logger.Information("No existing settings file found, using defaults");
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to load settings");
        }
    }

    public void Save()
    {
        try
        {
            var json = JsonConvert.SerializeObject(this, Formatting.Indented);
            File.WriteAllText(_settingsFile, json);
            _logger.Information("Settings saved successfully");
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to save settings");
        }
    }
}
