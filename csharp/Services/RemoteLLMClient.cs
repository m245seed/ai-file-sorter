using AiFileSorter.Models;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace AiFileSorter.Services;

public class RemoteLLMClient : ILLMClient
{
    private readonly HttpClient _httpClient;
    private readonly string _apiKey;
    private readonly Serilog.ILogger _logger;

    public RemoteLLMClient(string apiKey)
    {
        _httpClient = new HttpClient();
        _apiKey = apiKey;
        _logger = Logger.GetLogger("llm_logger");
    }

    public async Task<(string category, string subcategory)> CategorizeFileAsync(
        string fileName, 
        FileType fileType, 
        string? filePath = null, 
        bool useSubcategories = true)
    {
        try
        {
            _logger.Debug("Categorizing {FileType}: {FileName}", fileType.ToDisplayString(), fileName);

            var prompt = BuildPrompt(fileName, fileType, filePath, useSubcategories);
            var response = await CallOpenAIAsync(prompt);
            return ParseResponse(response);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to categorize file via remote LLM: {FileName}", fileName);
            return ("Uncategorized", string.Empty);
        }
    }

    private string BuildPrompt(string fileName, FileType fileType, string? filePath, bool useSubcategories)
    {
        var typeStr = fileType.ToDisplayString().ToLower();
        var pathContext = !string.IsNullOrEmpty(filePath) ? $" Path: {filePath}." : "";

        if (useSubcategories)
        {
            return $@"Categorize this {typeStr} into a category and subcategory based on its name.{pathContext}
{typeStr.Substring(0, 1).ToUpper() + typeStr.Substring(1)}: {fileName}

Respond with ONLY the category and subcategory in this exact format:
Category: <category_name>
Subcategory: <subcategory_name>

Be concise and use common category names.";
        }
        else
        {
            return $@"Categorize this {typeStr} based on its name.{pathContext}
{typeStr.Substring(0, 1).ToUpper() + typeStr.Substring(1)}: {fileName}

Respond with ONLY the category in this exact format:
Category: <category_name>

Be concise and use common category names.";
        }
    }

    private async Task<string> CallOpenAIAsync(string prompt)
    {
        var requestBody = new
        {
            model = "gpt-4o-mini",
            messages = new[]
            {
                new { role = "system", content = "You are a helpful assistant that categorizes files and directories." },
                new { role = "user", content = prompt }
            },
            max_tokens = 150,
            temperature = 0.3
        };

        var json = JsonConvert.SerializeObject(requestBody);
        var content = new StringContent(json, Encoding.UTF8, "application/json");

        _httpClient.DefaultRequestHeaders.Clear();
        _httpClient.DefaultRequestHeaders.Add("Authorization", $"Bearer {_apiKey}");

        var response = await _httpClient.PostAsync("https://api.openai.com/v1/chat/completions", content);
        response.EnsureSuccessStatusCode();

        var responseBody = await response.Content.ReadAsStringAsync();
        var jsonResponse = JObject.Parse(responseBody);
        var messageContent = jsonResponse["choices"]?[0]?["message"]?["content"]?.ToString();

        return messageContent ?? string.Empty;
    }

    private (string category, string subcategory) ParseResponse(string response)
    {
        var category = "Uncategorized";
        var subcategory = string.Empty;

        var lines = response.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        
        foreach (var line in lines)
        {
            if (line.StartsWith("Category:", StringComparison.OrdinalIgnoreCase))
            {
                category = line.Substring("Category:".Length).Trim();
            }
            else if (line.StartsWith("Subcategory:", StringComparison.OrdinalIgnoreCase))
            {
                subcategory = line.Substring("Subcategory:".Length).Trim();
            }
        }

        return (category, subcategory);
    }
}
