#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define COPY_FROM_HOST 0x10303
#define COPY_TO_HOST 0x10505

#define errExit(msg)                                                           \
  do {                                                                         \
    printf("%d\n", errno);                                                     \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define BUF_SIZE (1024 * 4) /* Maximum size for exchanged string */

struct intermem {
  uint8_t buf[BUF_SIZE];
  // size_t size;
};

struct intermem im;

void *sb_malloc(size_t size) { return malloc(size); }

int print_from_host(const char *words) {
  printf("made it here in the sandbox\n");
  printf("words: %p\n", words);
  printf("%s\n", words);
  return 0;
}

int main(int argc, char **argv) {
  fprintf(stderr, "%s\n", "things initialized");
  return 0;
}
