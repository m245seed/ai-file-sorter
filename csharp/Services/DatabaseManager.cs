using AiFileSorter.Models;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;

namespace AiFileSorter.Services;

public class DatabaseManager
{
    private readonly string _dbPath;
    private readonly Serilog.ILogger _logger;

    public DatabaseManager(string configDir)
    {
        _logger = Logger.GetLogger("db_logger");
        _dbPath = Path.Combine(configDir, "categorizations.db");
        InitializeDatabase();
    }

    private void InitializeDatabase()
    {
        try
        {
            using var connection = new SqliteConnection($"Data Source={_dbPath}");
            connection.Open();

            var createTableCommand = connection.CreateCommand();
            createTableCommand.CommandText = @"
                CREATE TABLE IF NOT EXISTS categorizations (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    file_path TEXT NOT NULL UNIQUE,
                    file_name TEXT NOT NULL,
                    file_type TEXT NOT NULL,
                    category TEXT NOT NULL,
                    subcategory TEXT,
                    taxonomy_id INTEGER DEFAULT 0,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )";
            createTableCommand.ExecuteNonQuery();

            _logger.Information("Database initialized at {DbPath}", _dbPath);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to initialize database");
            throw;
        }
    }

    public void SaveCategorization(CategorizedFile file)
    {
        try
        {
            using var connection = new SqliteConnection($"Data Source={_dbPath}");
            connection.Open();

            var command = connection.CreateCommand();
            command.CommandText = @"
                INSERT OR REPLACE INTO categorizations (file_path, file_name, file_type, category, subcategory, taxonomy_id)
                VALUES (@filePath, @fileName, @fileType, @category, @subcategory, @taxonomyId)";

            command.Parameters.AddWithValue("@filePath", file.FilePath);
            command.Parameters.AddWithValue("@fileName", file.FileName);
            command.Parameters.AddWithValue("@fileType", file.Type.ToString());
            command.Parameters.AddWithValue("@category", file.Category);
            command.Parameters.AddWithValue("@subcategory", file.Subcategory ?? (object)DBNull.Value);
            command.Parameters.AddWithValue("@taxonomyId", file.TaxonomyId);

            command.ExecuteNonQuery();
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to save categorization for {FilePath}", file.FilePath);
        }
    }

    public CategorizedFile? GetCategorization(string filePath)
    {
        try
        {
            using var connection = new SqliteConnection($"Data Source={_dbPath}");
            connection.Open();

            var command = connection.CreateCommand();
            command.CommandText = @"
                SELECT file_path, file_name, file_type, category, subcategory, taxonomy_id
                FROM categorizations
                WHERE file_path = @filePath";
            command.Parameters.AddWithValue("@filePath", filePath);

            using var reader = command.ExecuteReader();
            if (reader.Read())
            {
                return new CategorizedFile
                {
                    FilePath = reader.GetString(0),
                    FileName = reader.GetString(1),
                    Type = Enum.Parse<FileType>(reader.GetString(2)),
                    Category = reader.GetString(3),
                    Subcategory = reader.IsDBNull(4) ? string.Empty : reader.GetString(4),
                    TaxonomyId = reader.GetInt32(5)
                };
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to get categorization for {FilePath}", filePath);
        }

        return null;
    }

    public List<CategorizedFile> GetAllCategorizations()
    {
        var categorizations = new List<CategorizedFile>();

        try
        {
            using var connection = new SqliteConnection($"Data Source={_dbPath}");
            connection.Open();

            var command = connection.CreateCommand();
            command.CommandText = @"
                SELECT file_path, file_name, file_type, category, subcategory, taxonomy_id
                FROM categorizations
                ORDER BY created_at DESC";

            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                categorizations.Add(new CategorizedFile
                {
                    FilePath = reader.GetString(0),
                    FileName = reader.GetString(1),
                    Type = Enum.Parse<FileType>(reader.GetString(2)),
                    Category = reader.GetString(3),
                    Subcategory = reader.IsDBNull(4) ? string.Empty : reader.GetString(4),
                    TaxonomyId = reader.GetInt32(5)
                });
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to get all categorizations");
        }

        return categorizations;
    }

    public void DeleteCategorization(string filePath)
    {
        try
        {
            using var connection = new SqliteConnection($"Data Source={_dbPath}");
            connection.Open();

            var command = connection.CreateCommand();
            command.CommandText = "DELETE FROM categorizations WHERE file_path = @filePath";
            command.Parameters.AddWithValue("@filePath", filePath);
            command.ExecuteNonQuery();

            _logger.Debug("Deleted categorization for {FilePath}", filePath);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to delete categorization for {FilePath}", filePath);
        }
    }
}
