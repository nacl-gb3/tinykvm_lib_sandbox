#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define errExit(msg)                                                           \
  do {                                                                         \
    printf("%d\n", errno);                                                     \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

void *sb_malloc(size_t size) { return malloc(size); }

int print_from_host(char *words) {
  printf("words: %p\n", words);
  printf("%s\n", words);
  strlcpy(words, "hello fr in the sandbox", 22);
  return 0;
}

int main(int argc, char **argv) {
  fprintf(stderr, "%s\n", "things initialized");
  return 0;
}
