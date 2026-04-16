# basu - Lightweight sd-bus Library for Zephyr

## Overview

basu is a lightweight implementation of the systemd sd-bus API, ported to work with Zephyr RTOS. It provides D-Bus communication capabilities for embedded systems.

## Module Structure

```
basu/
├── CMakeLists.txt          # Main build configuration (modular)
├── zephyr/
│   ├── module.yml          # Zephyr module definition
│   ├── Kconfig             # Configuration options
│   ├── CMakeLists.txt      # Zephyr-specific build (legacy, kept for compatibility)
│   ├── include/            # Public headers
│   ├── tool/               # Zephyr compatibility shims
│   └── src/                # Zephyr-specific sources
├── include/                # Public API headers
│   └── systemd/            # systemd-compatible API
└── src/                    # Source code
    ├── basic/              # Core utilities
    ├── libsystemd/         # systemd API implementations
    │   ├── sd-bus/         # D-Bus implementation
    │   ├── sd-daemon/      # Service notification
    │   ├── sd-id128/       # ID generation
    │   └── sd-event/       # Event loop
    └── systemd/            # Public API definitions
```

## Configuration Options

Enable basu in your Zephyr project by adding to `prj.conf`:

```kconfig
CONFIG_BASU=y
CONFIG_BASU_LOG_LEVEL=3          # 0=OFF, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG
CONFIG_BASU_ENABLE_DAEMON=y      # Enable sd-daemon support
CONFIG_BASU_ENABLE_ID128=y       # Enable sd-id128 support
```

## Features

- **sd-bus**: Full D-Bus client library implementation
- **sd-event**: Event loop for asynchronous operations
- **sd-daemon**: Service notification API (optional)
- **sd-id128**: Unique ID generation (optional)

## Dependencies

- Zephyr POSIX API (`CONFIG_POSIX_API=y`)
- Network sockets (`CONFIG_NET_SOCKETS=y`)

## Usage Example

```c
#include <systemd/sd-bus.h>

sd_bus *bus = NULL;
sd_bus_message *reply = NULL;

// Connect to system bus
sd_bus_open_system(&bus);

// Call a method
sd_bus_call_method(bus,
    "org.freedesktop.DBus",   // service
    "/org/freedesktop/DBus",  // object path
    "org.freedesktop.DBus",   // interface
    "ListNames",              // method
    NULL,                     // error
    &reply,                   // reply
    "");                      // signature

// Process reply...

sd_bus_message_unref(reply);
sd_bus_unref(bus);
```

## Recent Improvements

### Socket Write Retry Mechanism

The library now includes intelligent retry logic for socket write operations in Zephyr environments:

- **Automatic retry**: Up to 3 attempts with exponential backoff (100μs → 200μs → 400μs)
- **Handles EAGAIN**: Properly handles Zephyr's EAGAIN semantics ("No more contexts")
- **Clean error reporting**: Clears errno on success to avoid stale error codes

This resolves issues with transient resource contention during initial socket operations.

## Building

The module integrates seamlessly with Zephyr's build system:

```bash
west build -b <your_board> <your_app>
```

## License

SPDX-License-Identifier: Apache-2.0 / LGPL-2.1-or-later

## References

- [systemd sd-bus documentation](https://www.freedesktop.org/software/systemd/man/sd-bus.html)
- [Zephyr Project](https://www.zephyrproject.org/)
