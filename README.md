[![C++ CI](https://github.com/sopranoworks/CodebookRuntimeCpp-Config/actions/workflows/ci.yml/badge.svg)](https://github.com/sopranoworks/CodebookRuntimeCpp-Config/actions)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/sopranoworks/CodebookRuntimeCpp-Config/releases/tag/v1.0.0)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# CodebookRuntimeCpp-Config

A lightweight, header-only HOCON parser for C++17, designed for high-reliability environments.

## Features

- **HOCON & JSON support** — parses standard JSON as well as HOCON-specific syntax
- **Path-based keys** — `a.b.c = 1` automatically creates nested objects
- **Object merging** — repeated keys are deep-merged rather than overwritten
- **Array concatenation** — `list += value` appends to an existing array
- **Variable substitution** — `${path}` and `${?path}` (optional) resolved after parsing
- **Exception-free API** — every operation returns `std::optional`; no exceptions are thrown
- **Header-only** — drop the `include/` directory into any project, no build step required

## Requirements

- C++17 or higher
- CMake 3.16+ (for building tests only)

## Installation

Copy or symlink the `include/` directory into your project and add it to your include path:

```cmake
target_include_directories(your_target PRIVATE path/to/CodebookRuntimeCpp-Config/include)
```

No linking is required — the library is entirely header-only.

## Usage

```cpp
#include <CodebookRuntime/Config/Config.h>

using namespace CodebookRuntime::Config;

// Parse from a string
auto cfg = Config::parse(R"(
    server {
        host = localhost
        port = 8080
    }
    debug = false
    tags = [api, v2]

    base_port = ${server.port}
)");

if (!cfg) {
    // Tokenisation, parse, or resolution error
    return;
}

// Dot-path access
auto host = cfg->get("server.host");   // std::optional<Value>
auto port = cfg->get("server.port");   // std::optional<Value>

if (host) std::cout << host->as<std::string>().value() << "\n";  // localhost
if (port) std::cout << port->as<int64_t>().value()    << "\n";  // 8080

// Substitution is fully resolved
auto base = cfg->get("base_port");
// base->as<int64_t>() == 8080

// Load from a file
auto file_cfg = Config::from_file("/etc/myapp/config.conf");
if (!file_cfg) { /* file not found or parse error */ }
```

### Supported value types

| HOCON / JSON | C++ type via `as<T>()` |
|---|---|
| `null` | `std::monostate` |
| `true` / `false` | `bool` |
| Integer | `int64_t` |
| Float | `double` |
| `"quoted"` / `unquoted` | `std::string` |
| `[...]` | `Array` (`std::vector<Value>`) |
| `{...}` | `Object` (`std::map<std::string, Value>`) |

## Testing

```bash
mkdir build && cd build
cmake ..
make
ctest
```

All tests are discovered automatically via `gtest_discover_tests`. Expected output:

```
100% tests passed, 0 tests failed out of 27
```

## Architecture

Parsing is a three-stage pipeline, each stage returning `std::nullopt` on error:

```
std::string_view
      │
      ▼
  [ Lexer ]          Lexer.h
      │  std::vector<Token>
      ▼
  [ Parser ]         Parser.h
      │  Value tree (may contain Placeholder nodes)
      ▼
  [ Resolver ]       Config.h
      │  Fully resolved Value tree
      ▼
  std::optional<Value>
```

| Component | Header | Responsibility |
|---|---|---|
| `Lexer` | `Lexer.h` | Tokenises input; strips `//` and `#` comments; emits `${`, `+=`, newlines |
| `Parser` | `Parser.h` | Recursive descent; handles dotted paths, object merging, `+=` |
| `Resolver` | `Config.h` | Depth-limited substitution pass; supports forward references and `${?...}` |
| `Value` | `Value.h` | `std::variant`-backed node; provides `as<T>()`, `get(path)`, `merge()` |
| `Config` | `Config.h` | Entry points: `Config::parse()`, `Config::from_file()`, `Config::resolve()` |

## Namespace

All types live in `CodebookRuntime::Config`.

```cpp
using namespace CodebookRuntime::Config;

Value v(int64_t{42});
Object obj;
Array  arr;
```
