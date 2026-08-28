# Testing Guide

## Overview

The wasteland2d project uses CMake's CTest integration for running a comprehensive test suite. All tests are standalone executables that can be run individually or as a complete suite.

## Test Categories

### Headless Tests (No External Dependencies)
- `attachment_test` - Entity attachment system
- `registry_reference_safety_test` - ECS reference safety

### JSON-Dependent Tests (Require nlohmann::json)
- `inventory_test` - Grid inventory, stacking, weight limits
- `equipment_test` - Equipment slots, weapon switching
- `encumbrance_test` - Weight-based movement penalties
- `dragdrop_test` - UI drag-and-drop, rotation, swapping
- `useitem_test` - Item usage system

### Physics-Dependent Tests (Require Box2D)
- `combat_test` - Hitscan combat, damage, death, corpse spawning
- `melee_test` - Melee combat arc system
- `kill_scenario_test` - End-to-end combat scenario

### SDL-Dependent Tests
- `font_runtime_test` - Text rendering and font loading

## Building Tests

Tests are built automatically when building the main project:

```bash
cmake -B build -S .
cmake --build build --config Debug
```

On Windows with MSVC:
```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

## Running Tests

### Run All Tests
```bash
cd build
ctest -C Debug --output-on-failure
```

### Run Specific Test
```bash
cd build
ctest -C Debug -R attachment_test --output-on-failure
```

### Run Tests with Verbose Output
```bash
cd build
ctest -C Debug --verbose
```

### Run Individual Test Executable
```bash
# On Windows
.\build\Debug\attachment_test.exe

# On Linux
./build/attachment_test
```

## Test Output

When tests pass, you'll see output like:
```
Test project D:/coding/cpp/wasteland2d/build
      Start  1: attachment_test
 1/11 Test  #1: attachment_test ..................   Passed    0.04 sec
...
100% tests passed, 0 tests failed out of 11
```

When tests fail, CTest will show detailed output including which assertion failed and where.

## Cross-Platform Compatibility

Tests are designed to work on both Windows and Linux:

- **Temporary files**: Tests use a cross-platform temporary directory (`C:\Users\user\AppData\Local\Temp` on Windows, `/tmp` on Linux)
- **Path separators**: Handled automatically by the `test_common.h` helper
- **Line endings**: All test files use CRLF for Windows compatibility

## Adding New Tests

To add a new test:

1. Create your test file in the `tests/` directory
2. Add it to `CMakeLists.txt` using the `add_wasteland_test` helper:
```cmake
add_wasteland_test(my_test tests/my_test.cpp TRUE)  # TRUE if it needs game/ headers
target_link_libraries(my_test PRIVATE engine nlohmann_json::nlohmann_json)  # Add required libraries
target_sources(my_test PRIVATE game/data/ItemDatabase.cpp)  # Add if it needs ItemDatabase
```

## Test Dependencies

Tests are organized by their dependencies:

- **Engine-only tests**: Link only against `engine`
- **Game tests**: Link against `engine` and include `game/` headers
- **JSON tests**: Also link against `nlohmann_json::nlohmann_json` and include `ItemDatabase.cpp`
- **Physics tests**: Also link against `box2d::box2d` and `glm::glm`
- **SDL tests**: Also link against SDL2 libraries

## Continuous Integration

The test suite is designed to be CI-friendly:

- All tests are headless (except `font_runtime_test` which can be skipped if needed)
- Tests use temporary files and don't modify project assets
- CTest provides standardized output for CI systems
- Tests are fast (complete suite runs in < 1 second)

## Troubleshooting

### Permission Issues on Windows
If tests fail due to permission issues with the temp directory, ensure the user has write access to `%TEMP%`.

### Missing Dependencies
If tests fail to build, ensure all dependencies are installed via vcpkg or system package manager.

### Asset Path Issues
Tests create their own temporary test data files and don't depend on project assets, so asset path issues should not occur.