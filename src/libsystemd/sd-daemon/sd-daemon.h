/* SPDX-License-Identifier: LGPL-2.1+ */
#ifndef foosddaemonhfoo
#define foosddaemonhfoo
/*
  Helper call for identifying a passed file descriptor. Returns 1 if
  the file descriptor is a socket of the specified family (AF_INET,
  ...) and type (SOCK_DGRAM, SOCK_STREAM, ...), 0 otherwise. If
  family is 0 a socket family check will not be done. If type is 0 a
  socket type check will not be done and the call only verifies if
  the file descriptor refers to a socket. If listening is > 0 it is
  verified that the socket is in listening mode. (i.e. listen() has
  been called) If listening is == 0 it is verified that the socket is
  not in listening mode. If listening is < 0 no listening mode check
  is done. Returns a negative errno style error code on failure.

  See sd_is_socket(3) for more information.
*/
int sd_is_socket(int fd, int family, int type, int listening);

/* The first passed file descriptor is fd 3 */
// #define SD_LISTEN_FDS_START 3
extern int SD_LISTEN_FDS_START;

/*
  Returns how many file descriptors have been passed, or a negative
  errno code on failure. Optionally, removes the $LISTEN_FDS,
  $LISTEN_PID, $LISTEN_PIDFDID, and $LISTEN_FDNAMES variables from the
  environment (recommended, but problematic in threaded environments).
  If r is the return value of this function you'll find the file
  descriptors passed as fds SD_LISTEN_FDS_START to
  SD_LISTEN_FDS_START+r-1. Returns a negative errno style error code on
  failure. This function call ensures that the FD_CLOEXEC flag is set
  for the passed file descriptors, to make sure they are not passed on
  to child processes. If FD_CLOEXEC shall not be set, the caller needs
  to unset it after this call for all file descriptors that are used.

  See sd_listen_fds(3) for more information.
*/
int sd_listen_fds(int unset_environment);

/*
  Helper call for identifying a passed file descriptor. Returns 1 if
  the file descriptor is an Internet socket, of the specified family
  (either AF_INET or AF_INET6) and the specified type (SOCK_DGRAM,
  SOCK_STREAM, ...), 0 otherwise. If version is 0 a protocol version
  check is not done. If type is 0 a socket type check will not be
  done. If port is 0 a socket port check will not be done. The
  listening flag is used the same way as in sd_is_socket(). Returns a
  negative errno style error code on failure.

  See sd_is_socket_inet(3) for more information.
*/
int sd_is_socket_inet(int fd, int family, int type, int listening, uint16_t port);

#endif
