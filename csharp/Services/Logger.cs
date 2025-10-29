using System;
using System.Collections.Generic;
using System.IO;
using Serilog;
using Serilog.Core;

namespace AiFileSorter.Services;

public static class Logger
{
    private static readonly Dictionary<string, ILogger> _loggers = new();
    private static bool _initialized;

    public static void Setup()
    {
        if (_initialized) return;

        var logDirectory = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "AiFileSorter",
            "logs"
        );
        Directory.CreateDirectory(logDirectory);

        var logFilePath = Path.Combine(logDirectory, "aifilesorter.log");

        Log.Logger = new LoggerConfiguration()
            .MinimumLevel.Debug()
            .WriteTo.Console()
            .WriteTo.File(
                logFilePath,
                rollingInterval: RollingInterval.Day,
                retainedFileCountLimit: 7
            )
            .CreateLogger();

        _loggers["core_logger"] = Log.ForContext("LoggerName", "core");
        _loggers["ui_logger"] = Log.ForContext("LoggerName", "ui");
        _loggers["llm_logger"] = Log.ForContext("LoggerName", "llm");
        _loggers["db_logger"] = Log.ForContext("LoggerName", "db");

        _initialized = true;
        Log.Information("Logger system initialized");
    }

    public static ILogger GetLogger(string name)
    {
        if (!_initialized)
        {
            Setup();
        }

        if (_loggers.TryGetValue(name, out var logger))
        {
            return logger;
        }

        var newLogger = Log.ForContext("LoggerName", name);
        _loggers[name] = newLogger;
        return newLogger;
    }

    public static void Shutdown()
    {
        Log.CloseAndFlush();
    }
}
