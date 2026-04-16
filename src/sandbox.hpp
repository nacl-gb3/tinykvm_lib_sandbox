#ifndef SANDBOX_H
#define SANDBOX_H

#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define errExit(msg)                                                           \
  do {                                                                         \
    printf("%d\n", errno);                                                     \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define BUF_SIZE (1024 * 4) /* Maximum size for exchanged string */

/* Define a structure that will be imposed on the shared
   memory object */

struct intermem {
   uint8_t *buf; 
   size_t size;
};

struct shmbuf {
  // replace with monitors
  sem_t sem1;            /* POSIX unnamed semaphore */
  sem_t sem2;            /* POSIX unnamed semaphore */
  int fd;                /* file descriptior */
  struct intermem im;    /* memory buffer */
};


#define BASE_INTERMEM_SIZE 4096

int sandbox_run(uint8_t);

#endif
