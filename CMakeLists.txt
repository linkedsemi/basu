# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)
zephyr_library_named(basu)
zephyr_library_compile_options(-msmall-data-limit=0 -Wno-redundant-decls -Wno-nested-externs)

zephyr_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/systemd
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/basic
  ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd
  ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd/sd-bus
  ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd/sd-daemon
  ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd/sd-id128
  ${CMAKE_CURRENT_SOURCE_DIR}/src/systemd
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../dbus-broker/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../dbus-broker/subprojects/libclist-3/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../dbus-broker/subprojects/libcstdaux-1/src
)

zephyr_library_compile_definitions(
  __ZEPHYR__=1
  BASU_BUILD_ZEPHYR=1
  _FILE_OFFSET_BITS=64
  _GNU_SOURCE
  __LINUX_ERRNO_EXTENSIONS__
  GPERF_LEN_TYPE=size_t
  ENABLE_DEBUG_HASHMAP=0
  _POSIX_C_SOURCE=200809L
  _XOPEN_SOURCE=700
  _DEFAULT_SOURCE
)

# ---- shims & tool glue----
zephyr_library_sources(
  zephyr/tool/flockfile.c
  zephyr/tool/sys_fun.c
  zephyr/src/sys_mmap.c
)

# Create symlink for sd-bus.h to fix include path issue
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd/sd-bus/sd-bus.h
  COMMAND ln -sf ../../systemd/sd-bus.h ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd/sd-bus/sd-bus.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/systemd/sd-bus.h
  COMMENT "Creating symlink for sd-bus.h"
)

# Add the symlink as a dependency
add_custom_target(create_sd_bus_symlink ALL
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd/sd-bus/sd-bus.h
)

# Make basu library depend on the symlink creation
add_dependencies(basu create_sd_bus_symlink)

# ===== src/basic =====
zephyr_library_sources(
  src/basic/alloc-util.c
  src/basic/audit-util.c
  src/basic/bus-label.c
  src/basic/env-util.c
  src/basic/errno-list.c
  src/basic/escape.c
  src/basic/fd-util.c
  src/basic/fileio.c
  src/basic/gunicode.c
  src/basic/hash-funcs.c
  src/basic/hashmap.c
  src/basic/hexdecoct.c
  src/basic/io-util.c
  src/basic/json.c
  src/basic/locale-util.c
  src/basic/log.c
  src/basic/memfd-util.c
  src/basic/parse-util.c
  src/basic/path-util.c
  src/basic/prioq.c
  src/basic/process-util.c
  src/basic/random-util.c
  src/basic/siphash24.c
  src/basic/socket-util.c
  src/basic/string-util.c
  src/basic/strv.c
  src/basic/syslog-util.c
  src/basic/terminal-util.c
  src/basic/time-util.c
  src/basic/user-util.c
  src/basic/utf8.c
  src/basic/util.c
  src/basic/verbs.c
  src/basic/xml.c
)

# ===== src/libsystemd/sd-bus =====
zephyr_library_sources(
  src/libsystemd/sd-bus/bus-common-errors.c
  src/libsystemd/sd-bus/bus-control.c
  src/libsystemd/sd-bus/bus-convenience.c
  src/libsystemd/sd-bus/bus-dump.c
  src/libsystemd/sd-bus/bus-error.c
  src/libsystemd/sd-bus/bus-gvariant.c
  src/libsystemd/sd-bus/bus-internal.c
  src/libsystemd/sd-bus/bus-introspect.c
  src/libsystemd/sd-bus/bus-kernel.c
  src/libsystemd/sd-bus/bus-match.c
  src/libsystemd/sd-bus/bus-message.c
  src/libsystemd/sd-bus/bus-objects.c
  src/libsystemd/sd-bus/bus-signature.c
  src/libsystemd/sd-bus/bus-slot.c
  src/libsystemd/sd-bus/bus-socket.c
  src/libsystemd/sd-bus/bus-track.c
  src/libsystemd/sd-bus/bus-type.c
  src/libsystemd/sd-bus/sd-bus.c
)

# ===== sd-id128 =====
zephyr_library_sources(
  src/libsystemd/sd-id128/id128-util.c
  src/libsystemd/sd-id128/sd-id128.c
)

# ===== sd-daemon =====
zephyr_library_sources(
  src/libsystemd/sd-daemon/sd-daemon.c
)

# ===== sd-event =====
zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/libsystemd/sd-event
)

zephyr_library_sources(
  src/libsystemd/sd-event/sd-event.c
)

target_compile_options(basu PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/include/head.h
)

# 告警/编译选项
target_compile_options(basu PRIVATE
  -U_ANSI_SOURCE
  -Werror=undef -Wlogical-op -Wmissing-include-dirs -Wold-style-definition
  -Wpointer-arith -Winit-self -Wfloat-equal -Wsuggest-attribute=noreturn
  -Werror=missing-prototypes -Werror=implicit-function-declaration
  -Werror=missing-declarations -Werror=return-type
  -Werror=incompatible-pointer-types -Werror=format=2
  -Wstrict-prototypes -Wredundant-decls -Wmissing-noreturn
  -Wimplicit-fallthrough=5 -Wshadow -Wendif-labels -Wstrict-aliasing=2
  -Wwrite-strings -Werror=overflow -Werror=shift-count-overflow
  -Werror=shift-overflow=2 -Wdate-time -Wnested-externs
  -Wno-unused-parameter -Wno-missing-field-initializers
  -Wno-unused-result -Wno-format-signedness -Wno-format-nonliteral
  -Wno-error=nonnull -Wno-maybe-uninitialized
)

zephyr_compile_options(-std=gnu11)

# ===== busctl =====
zephyr_library_sources(
  src/busctl/busctl.c
  src/busctl/busctl-introspect.c
)

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/busctl/busctl.c
  PROPERTIES COMPILE_DEFINITIONS "main=basu_busctl_entry;exit=busctl_thread_exit"
)