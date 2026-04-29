#ifndef SANDBOX_H
#define SANDBOX_H

#include <semaphore.h>
#include <span>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tinykvm/machine.hpp>

#define errExit(msg)                                                           \
  do {                                                                         \
    printf("%d\n", errno);                                                     \
    perror(msg);                                                               \
    shm_uninit();                                                              \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define BUF_SIZE (1024 * 4) /* Maximum size for exchanged string */

/* Define a structure that will be imposed on the shared
   memory object */

struct intermem {
  uint8_t buf[BUF_SIZE];
  // size_t size;
};

struct shmbuf {
  // replace with monitors
  sem_t sem1; /* POSIX unnamed semaphore */
  sem_t sem2; /* POSIX unnamed semaphore */
  int fd;     /* file descriptior */
  struct intermem im;
};

struct sandbox_buf {
  uint64_t gva;
  std::span<uint8_t> buf;
};

#define BASE_INTERMEM_SIZE 4096

struct sandbox_buf *malloc_in_sandbox(tinykvm::Machine&, size_t);
int free_in_sandbox(tinykvm::Machine&, uint64_t);

int shm_init();
int sandbox_run();
struct shmbuf *get_shm_obj();
int shm_uninit();

#endif
