# Logging System

## Overview

The wasteland2d project includes a professional logging system with colored console output, timestamps, log levels, and optional file logging. The logging system is automatically initialized and shut down by the Application class.

## Features

- **Cross-platform colored console output** - Windows (SetConsoleTextAttribute) and Unix (ANSI codes)
- **Timestamps** - Millisecond precision timestamps
- **Log levels** - Debug, Info, Warning, Error, Fatal
- **Categories** - System-specific tags (Core, Combat, Physics, Inventory, etc.)
- **File output** - Optional file logging with automatic flushing
- **Level filtering** - Only show logs at or above configured level
- **Zero-overhead when disabled** - Compile-time elimination of disabled logs

## Configuration

The logging system is configured via `ApplicationConfig`:

```cpp
engine::ApplicationConfig config;
config.loggerConfig.level = engine::LogLevel::Info;
config.loggerConfig.enableColors = true;
config.loggerConfig.enableFileOutput = false;
config.loggerConfig.logFilePath = "wasteland2d.log";
config.loggerConfig.enableTimestamps = true;
config.loggerConfig.enableCategories = true;
```

### Configuration Options

- **level** - Minimum log level to output (Debug, Info, Warning, Error, Fatal)
- **enableColors** - Enable colored console output (default: true)
- **enableFileOutput** - Enable file logging (default: false)
- **logFilePath** - Path to log file (default: "wasteland2d.log")
- **enableTimestamps** - Include timestamps in output (default: true)
- **enableCategories** - Include category tags (default: true)

## Usage

### Basic Logging

```cpp
#include "core/Logger.h"

// Simple string logging
LOG_INFO(engine::LogCategory::Core, "Application initialized");
LOG_WARNING(engine::LogCategory::Assets, "Texture not found, using fallback");
LOG_ERROR(engine::LogCategory::Network, "Connection failed");
```

### Log Levels

```cpp
LOG_DEBUG(engine::LogCategory::Physics, "Entity position updated");
LOG_INFO(engine::LogCategory::Combat, "Hit for 25 damage");
LOG_WARNING(engine::LogCategory::Inventory, "Inventory nearly full");
LOG_ERROR(engine::LogCategory::Render, "Shader compilation failed");
LOG_FATAL(engine::LogCategory::Core, "Critical initialization failure");
```

### Variadic Arguments

The logger supports space-separated arguments (simple stream-style):

```cpp
LOG_INFO(engine::LogCategory::Combat, "hit for", damage, "damage");
LOG_INFO(engine::LogCategory::Inventory, "took", stacksBefore, "stacks");
```

For more complex formatting, use std::ostringstream:

```cpp
std::ostringstream msg;
msg << "hit for " << damage << " damage";
if (killed) msg << " -- target down";
LOG_INFO(engine::LogCategory::Combat, msg.str().c_str());
```

## Log Categories

Predefined categories for common systems:

```cpp
engine::LogCategory::General
engine::LogCategory::Core
engine::LogCategory::ECS
engine::LogCategory::Physics
engine::LogCategory::Render
engine::LogCategory::Input
engine::LogCategory::Audio
engine::LogCategory::Assets
engine::LogCategory::Combat
engine::LogCategory::Inventory
engine::LogCategory::UI
engine::LogCategory::Network
engine::LogCategory::FileIO
```

## Output Format

Standard output format (with all features enabled):

```
[HH:MM:SS.mmm] [LEVEL] [CATEGORY] message
```

Example:
```
[06:50:35.292] [INFO ] [Core] Initializing application
[06:50:36.509] [INFO ] [Combat] hit for 25 damage
[06:50:37.275] [INFO ] [Combat] hit for 25 damage -- target down
```

## Color Scheme

Console colors (when enabled):

- **Debug** - Gray
- **Info** - Green
- **Warning** - Yellow
- **Error** - Red
- **Fatal** - Bright Red

## Performance Considerations

- Logs are filtered at compile-time when below configured level
- String formatting is only performed if the log level will be output
- File output is buffered and flushed on critical messages
- Use `Logger::shouldLog()` for expensive operations:

```cpp
if (Logger::shouldLog(LogLevel::Debug)) {
    // Expensive debug-only computation
    auto result = expensiveComputation();
    LOG_DEBUG(LogCategory::Physics, "Result:", result);
}
```

## Integration with Application

The logging system is automatically initialized in the Application constructor and shut down in the destructor. No manual initialization is required in normal use.

```cpp
// Application.cpp - Automatic initialization
Application::Application(ApplicationConfig config) {
    Logger::init(config_.loggerConfig);
    // ... rest of initialization
}

Application::~Application() {
    Logger::shutdown();
    // ... rest of cleanup
}
```

## File Logging

To enable file logging:

```cpp
config.loggerConfig.enableFileOutput = true;
config.loggerConfig.logFilePath = "wasteland2d.log";
```

The log file includes timestamps, levels, categories, and messages (no colors in file output).

## Best Practices

1. **Use appropriate log levels**
   - Debug: Detailed debugging information
   - Info: General informational messages
   - Warning: Potentially problematic situations
   - Error: Error events that allow continuation
   - Fatal: Critical errors that terminate the application

2. **Use meaningful categories**
   - Categories help filter logs by system
   - Use predefined categories when possible
   - Consistent naming across the codebase

3. **Keep messages concise**
   - Log messages should be clear and actionable
   - Include relevant data (values, IDs, positions)
   - Avoid sensitive information (passwords, keys)

4. **Consider performance**
   - Avoid expensive operations in hot paths
   - Use `shouldLog()` for conditional expensive logging
   - Don't log every frame in release builds

## Future Enhancements

Potential improvements for future versions:

- Proper {}-style formatting (using fmt library)
- Thread ID in output
- File/line number context
- Category-based filtering
- Remote logging support
- Log rotation by size/time