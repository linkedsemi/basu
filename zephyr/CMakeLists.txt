if(CONFIG_BASU)
    zephyr_library_named(basu)

    # Include directories
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/include)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/basic)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/busctl)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/fundamental)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include/systemd)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-daemon)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-id128)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/systemd)


    # Add Zephyr compatibility sources
    zephyr_library_sources(
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/flockfile.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/sys_compat.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/sys_fun.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/string_fun.c
        )

    # Add essential basu source files

    # Basic utility sources
    zephyr_library_sources(
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/alloc-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/audit-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/bus-label.c
        # ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/cap-list.c
        # ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/capability-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/env-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/errno-list.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/escape.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/fd-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/fileio.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/fs-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/gunicode.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/hash-funcs.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/hashmap.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/hexdecoct.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/io-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/json.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/locale-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/log.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/memfd-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/parse-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/path-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/prioq.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/process-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/random-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/siphash24.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/socket-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/string-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/strv.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/syslog-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/terminal-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/time-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/user-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/utf8.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/verbs.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/xml.c

        ${ZEPHYR_CURRENT_MODULE_DIR}/src/basic/hmac.c
        )

    # sd-bus essential sources
    zephyr_library_sources(
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-common-errors.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-control.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-convenience.c
        # ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-creds.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-dump.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-error.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-gvariant.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-internal.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-introspect.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-kernel.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-match.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-message.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-objects.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-signature.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-slot.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-socket.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-track.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/bus-type.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-bus/sd-bus.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-daemon/sd-daemon.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-id128/id128-util.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-id128/sd-id128.c
        )

    zephyr_library_sources(
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/busctl/busctl-introspect.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/busctl/busctl.c
        )

    zephyr_library_sources(
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-journal/sd-journal.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/libsystemd/sd-event/sd-event.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/fundamental/sha256-fundamental.c
        )

    zephyr_include_directories(
        ${ZEPHYR_CURRENT_MODULE_DIR}/include
        )

endif()
