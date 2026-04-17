#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "sandbox.hpp"

#define COPY_FROM_HOST 0x10303
#define COPY_TO_HOST 0x10505

struct intermem im;

int main(int argc, char **argv) {
  int fd;
  struct shmbuf *shmp;

  fprintf(stderr, "check args\n");

  size_t im_page_cnt = (size_t)argv[1];

  printf("im_page_cnt sb: %ld\n", im_page_cnt);

  im.buf = (uint8_t *)calloc(im_page_cnt, BASE_INTERMEM_SIZE);
  im.size = im_page_cnt * BASE_INTERMEM_SIZE;

  // TODO: vmcall to get data from host
  int err = syscall(COPY_FROM_HOST, im.buf, im.size);
  if (err == -1) {
    errExit("COPY_FROM_HOST");
  }
  fprintf(stderr, "%s\n", im.buf);

  char str[11] = "hello back";
  memcpy(im.buf, str, 11);

  // TODO: vmcall to send data back to host
  err = syscall(COPY_TO_HOST, im.buf, 11);
  if (err == -1) {
    errExit("COPY_TO_HOST");
  }

  // if (sem_post(&shmp->sem1) == -1)
  //   errExit("sem_post");

  /* Wait until peer says that it has finished accessing
     the shared memory. \*/

  //  if (sem_wait(&shmp->sem2) == -1)
  //    errExit("sem_wait");

  /* Write modified data in shared memory to standard output. */

  return 0;
}
