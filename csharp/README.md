# AI File Sorter - C# Version

This is the C# version of AI File Sorter, a cross-platform desktop application built with .NET 9 and Avalonia UI.

## Overview

AI File Sorter is a powerful desktop application that automates file organization using AI. It categorizes and sorts files and folders based on their names and extensions.

## Requirements

- .NET 9.0 SDK or later
- Windows, macOS, or Linux

## Building from Source

1. Clone the repository:
```bash
git clone https://github.com/m245seed/ai-file-sorter.git
cd ai-file-sorter/csharp
```

2. Build the application:
```bash
dotnet build
```

3. Run the application:
```bash
dotnet run
```

## Publishing

To create a self-contained executable for your platform:

### Windows
```bash
dotnet publish -c Release -r win-x64 --self-contained
```

### macOS (Intel)
```bash
dotnet publish -c Release -r osx-x64 --self-contained
```

### macOS (Apple Silicon)
```bash
dotnet publish -c Release -r osx-arm64 --self-contained
```

### Linux
```bash
dotnet publish -c Release -r linux-x64 --self-contained
```

The published application will be in `bin/Release/net9.0/{runtime}/publish/`

## Features

- **AI-Powered Categorization**: Classify files intelligently using local or remote LLM
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **Intuitive Interface**: Modern Avalonia UI-based interface
- **Local Database Caching**: Speeds up repeated categorization
- **Sorting Preview**: See how files will be organized before confirming changes
- **Customizable Rules**: Configure categorization behavior

## Project Structure

```
csharp/
├── Models/              # Data models
├── Services/            # Business logic and services
│   ├── DatabaseManager.cs
│   ├── FileScanner.cs
│   ├── ILLMClient.cs
│   ├── LocalLLMClient.cs
│   ├── RemoteLLMClient.cs
│   ├── Logger.cs
│   └── Settings.cs
├── ViewModels/          # MVVM ViewModels
├── Views/               # UI Views (AXAML)
└── Assets/              # Images and resources
```

## Technology Stack

- **.NET 9.0**: Core framework
- **Avalonia UI 11.3**: Cross-platform UI framework
- **CommunityToolkit.Mvvm**: MVVM helpers
- **Microsoft.Data.Sqlite**: Database access
- **Newtonsoft.Json**: JSON serialization
- **Serilog**: Logging framework

## Usage

1. Launch the application
2. Select a directory to analyze
3. Configure options (use subcategories, categorize files/directories)
4. Click "Analyze" to scan and categorize files
5. Review the categorized files in the data grid
6. Click "Sort Files" to move files into category folders

## Configuration

Settings are stored in:
- **Windows**: `%LOCALAPPDATA%\AiFileSorter\settings.json`
- **macOS**: `~/Library/Application Support/AiFileSorter/settings.json`
- **Linux**: `~/.local/share/AiFileSorter/settings.json`

## Database

Categorization history is stored in a SQLite database at:
- **Windows**: `%LOCALAPPDATA%\AiFileSorter\categorizations.db`
- **macOS**: `~/Library/Application Support/AiFileSorter/categorizations.db`
- **Linux**: `~/.local/share/AiFileSorter/categorizations.db`

## Notes

- The current implementation uses a rule-based categorization system as a fallback
- For full LLM integration, you would need to:
  - For remote: Add OpenAI API key support
  - For local: Integrate llama.cpp via P/Invoke or use a managed wrapper

## License

This project is licensed under the GNU AFFERO GENERAL PUBLIC LICENSE (GNU AGPL). See the [LICENSE](../LICENSE) file for details.

## Original C++ Version

The original C++ version with GTK is available in the `app/` directory of this repository.
