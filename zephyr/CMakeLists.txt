zephyr_library()

# Set source directory
set(BASU_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../src)

# Include directories
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
zephyr_library_include_directories(${BASU_SOURCE_DIR})
zephyr_library_include_directories(${BASU_SOURCE_DIR}/basic)
zephyr_library_include_directories(${BASU_SOURCE_DIR}/libsystemd)

# Add Zephyr compatibility sources
zephyr_library_sources(
  tool/sys_compat.c
)

# Add essential basu source files

# Basic utility sources
zephyr_library_sources(
  ${BASU_SOURCE_DIR}/basic/alloc-util.c
  ${BASU_SOURCE_DIR}/basic/fd-util.c
  ${BASU_SOURCE_DIR}/basic/errno-list.c
  ${BASU_SOURCE_DIR}/basic/escape.c
  ${BASU_SOURCE_DIR}/basic/fileio.c
)

# sd-bus essential sources
zephyr_library_sources(
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-creds.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-control.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-error.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-internal.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-common-errors.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-convenience.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/bus-message.c
  ${BASU_SOURCE_DIR}/libsystemd/sd-bus/sd-bus.c
  ${BASU_SOURCE_DIR}/basic/log.c
  ${BASU_SOURCE_DIR}/basic/hash-funcs.c
  ${BASU_SOURCE_DIR}/basic/hexdecoct.c
  ${BASU_SOURCE_DIR}/basic/io-util.c
  ${BASU_SOURCE_DIR}/basic/parse-util.c
  ${BASU_SOURCE_DIR}/basic/path-util.c
  ${BASU_SOURCE_DIR}/basic/prioq.c
  ${BASU_SOURCE_DIR}/basic/process-util.c
  ${BASU_SOURCE_DIR}/basic/random-util.c
  ${BASU_SOURCE_DIR}/basic/siphash24.c
  ${BASU_SOURCE_DIR}/basic/socket-util.c
  ${BASU_SOURCE_DIR}/basic/string-util.c
  ${BASU_SOURCE_DIR}/basic/strv.c
  ${BASU_SOURCE_DIR}/basic/time-util.c
  ${BASU_SOURCE_DIR}/basic/utf8.c
  ${BASU_SOURCE_DIR}/basic/util.c
)
