zephyr_library_named(basu)

# Set source directory
set(BASU_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../src)
set(BASU_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# Include directories
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/include/sys)
zephyr_library_include_directories(${BASU_SOURCE_DIR})
zephyr_library_include_directories(${BASU_SOURCE_DIR}/basic)
zephyr_library_include_directories(${BASU_SOURCE_DIR}/busctl)
zephyr_library_include_directories(${BASU_SOURCE_DIR}/libsystemd/sd-bus)
zephyr_library_include_directories(${BASU_SOURCE_DIR}/libsystemd/sd-daemon)
zephyr_library_include_directories(${BASU_SOURCE_DIR}/systemd)
zephyr_library_include_directories(${BASU_DIR}/include/systemd)

# Add Zephyr compatibility sources
zephyr_library_sources(
  ${CMAKE_CURRENT_LIST_DIR}/tool/flockfile.c
  ${CMAKE_CURRENT_LIST_DIR}/tool/sys_compat.c
  ${CMAKE_CURRENT_LIST_DIR}/tool/sys_fun.c
)

# Add essential basu source files

# Basic utility sources
zephyr_library_sources(
  ${BASU_SOURCE_DIR}/basic/alloc-util.c
  ${BASU_SOURCE_DIR}/basic/audit-util.c
  ${BASU_SOURCE_DIR}/basic/bus-label.c
  # ${BASU_SOURCE_DIR}/basic/cap-list.c
  # ${BASU_SOURCE_DIR}/basic/capability-util.c
  ${BASU_SOURCE_DIR}/basic/env-util.c
  ${BASU_SOURCE_DIR}/basic/errno-list.c
  ${BASU_SOURCE_DIR}/basic/escape.c
  ${BASU_SOURCE_DIR}/basic/fd-util.c
  ${BASU_SOURCE_DIR}/basic/fileio.c
  ${BASU_SOURCE_DIR}/basic/fs-util.c
  ${BASU_SOURCE_DIR}/basic/gunicode.c
  ${BASU_SOURCE_DIR}/basic/hash-funcs.c
  ${BASU_SOURCE_DIR}/basic/hashmap.c
  ${BASU_SOURCE_DIR}/basic/hexdecoct.c
  ${BASU_SOURCE_DIR}/basic/io-util.c
  # ${BASU_SOURCE_DIR}/basic/json.c
  ${BASU_SOURCE_DIR}/basic/locale-util.c
  ${BASU_SOURCE_DIR}/basic/log.c
  ${BASU_SOURCE_DIR}/basic/memfd-util.c
  ${BASU_SOURCE_DIR}/basic/parse-util.c
  ${BASU_SOURCE_DIR}/basic/path-util.c
  ${BASU_SOURCE_DIR}/basic/prioq.c
  ${BASU_SOURCE_DIR}/basic/process-util.c
  ${BASU_SOURCE_DIR}/basic/random-util.c
  ${BASU_SOURCE_DIR}/basic/siphash24.c
  ${BASU_SOURCE_DIR}/basic/socket-util.c
  ${BASU_SOURCE_DIR}/basic/string-util.c
  ${BASU_SOURCE_DIR}/basic/strv.c
  ${BASU_SOURCE_DIR}/basic/syslog-util.c
  ${BASU_SOURCE_DIR}/basic/terminal-util.c
  ${BASU_SOURCE_DIR}/basic/time-util.c
  ${BASU_SOURCE_DIR}/basic/user-util.c
  ${BASU_SOURCE_DIR}/basic/utf8.c
  ${BASU_SOURCE_DIR}/basic/util.c
  ${BASU_SOURCE_DIR}/basic/verbs.c
  ${BASU_SOURCE_DIR}/basic/xml.c
)

# sd-bus essential sources
zephyr_library_sources(
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-common-errors.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-control.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-convenience.c
  # ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-creds.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-dump.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-error.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-gvariant.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-internal.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-introspect.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-kernel.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-match.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-message.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-objects.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-signature.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-slot.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-socket.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-track.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-type.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/sd-bus.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-daemon/sd-daemon.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-id128/id128-util.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-id128/sd-id128.c
)

zephyr_library_sources(
  ${BASU_SOURCE_DIR}/busctl/busctl-introspect.c
  ${BASU_SOURCE_DIR}/busctl/busctl.c
)

zephyr_library_sources(
  ${BASU_SOURCE_DIR}/libsystemd/sd-journal/sd-journal.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-event/sd-event.c
  )

zephyr_include_directories(${BASU_DIR}/include)
