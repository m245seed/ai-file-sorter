using AiFileSorter.Models;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

namespace AiFileSorter.Services;

public class FileScanner
{
    private readonly Serilog.ILogger _logger;

    public FileScanner()
    {
        _logger = Logger.GetLogger("core_logger");
    }

    public List<FileEntry> GetDirectoryEntries(string directoryPath, FileScanOptions options)
    {
        var fileEntries = new List<FileEntry>();
        _logger.Debug("Scanning directory '{DirectoryPath}' with options mask {Options}", 
            directoryPath, (int)options);

        try
        {
            var directory = new DirectoryInfo(directoryPath);
            if (!directory.Exists)
            {
                _logger.Warning("Directory does not exist: {DirectoryPath}", directoryPath);
                return fileEntries;
            }

            foreach (var entry in directory.GetFileSystemInfos())
            {
                var fullPath = entry.FullName;
                var fileName = entry.Name;
                var isHidden = IsFileHidden(entry);

                if (IsJunkFile(fileName))
                    continue;

                FileType fileType;
                bool shouldAdd = false;

                if (entry is DirectoryInfo)
                {
                    if (options.HasFlag(FileScanOptions.Directories) &&
                        (options.HasFlag(FileScanOptions.HiddenFiles) || !isHidden))
                    {
                        fileType = FileType.Directory;
                        shouldAdd = true;
                    }
                    else
                    {
                        continue;
                    }
                }
                else if (entry is FileInfo)
                {
                    if (options.HasFlag(FileScanOptions.Files) &&
                        (options.HasFlag(FileScanOptions.HiddenFiles) || !isHidden))
                    {
                        fileType = FileType.File;
                        shouldAdd = true;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }

                if (shouldAdd)
                {
                    fileEntries.Add(new FileEntry
                    {
                        FullPath = fullPath,
                        FileName = fileName,
                        Type = fileType
                    });
                }
            }

            _logger.Debug("Found {Count} entries in directory '{DirectoryPath}'", 
                fileEntries.Count, directoryPath);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Error scanning directory '{DirectoryPath}'", directoryPath);
        }

        return fileEntries;
    }

    private bool IsFileHidden(FileSystemInfo entry)
    {
        try
        {
            // Check file attributes
            if ((entry.Attributes & FileAttributes.Hidden) == FileAttributes.Hidden)
                return true;

            // On Unix-like systems, files starting with '.' are considered hidden
            if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                return entry.Name.StartsWith('.');
            }

            return false;
        }
        catch
        {
            return false;
        }
    }

    private bool IsJunkFile(string fileName)
    {
        // Skip common junk files
        var junkFiles = new[]
        {
            ".DS_Store",
            "Thumbs.db",
            "desktop.ini",
            ".directory"
        };

        return junkFiles.Contains(fileName, StringComparer.OrdinalIgnoreCase);
    }
}
