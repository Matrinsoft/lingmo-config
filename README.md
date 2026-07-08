# lingmo-config

Lingmo Desktop - Configuration system library.

## Overview

`lingmo-config` provides a layered key-value configuration system for Lingmo Desktop components. It supports:

- **INI format** configuration files
- **Layered resolution**: system → user → session override
- **File watching** via inotify for live configuration updates
- **Group-based** key organization

## Dependencies

- Qt 6 (Core)

## Building

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
ctest --test-dir build
```

## License

GPL-2.0-or-later
