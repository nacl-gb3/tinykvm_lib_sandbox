#include "assert.hpp"
#include "sandbox.hpp"
#include "shm_lib.h"
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static const char *PROGRAM_STATUS_MSG[] = {"Succeeded", "Invalid image",
                                           "Incomplete Image",
                                           "Memory allocation failure"};

#define MEMORY_ALLOC_ERR_MSG 3

struct sandbox {
  int fd;
  uint8_t *to_mem;
  uint8_t *from_mem;
};

/* Step 4: Create a manual wrapper around call to lib.h */
ImageHeader *sb_parse_image_header(struct sandbox *sb, char *in) {
  // write argument to memory
  return nullptr;
}

void sb_parse_image_body(struct sandbox *sb, char *in, ImageHeader *header,
                         OnProgress *on_progress, char *out) {
  parse_image_body(in, header, on_progress, out);
}

void image_parsing_progress(unsigned int progress) {
  std::cout << "Image parsing: " << progress << " out of 100\n";
}

void get_image_bytes(char *input_stream) {
  // Get the bytes of the image from the file into input stream
  // This is just a toy example, so we will leave this empty for now
}

// An example application that simulates a typical image parsing program
// The library simulates a typilcal image decoding library such as libjpeg
int main(int argc, char const *argv[]) {
  /* HIGH LEVEL IDEA */

  /* Step 0: Set up shared memory between the processes using mmap */
  int to_sandbox_fd = open("/tmp/tosandbox", O_CREAT);
  if (to_sandbox_fd == -1) {
    std::cerr << "Error: shared mem file creation failed\n";
    return errno;
  }

  if (ftruncate(to_sandbox_fd, 1024 * 4)) {
    std::cerr << "ftruncate failed\n";
    return errno;
  }

  int from_sandbox_fd = open("/tmp/fromsandbox", O_CREAT);
  if (from_sandbox_fd == -1) {
    std::cerr << "Error: shared mem file creation failed\n";
    return errno;
  }

  if (ftruncate(from_sandbox_fd, 1024 * 4)) {
    std::cerr << "from sandbox ftruncate failed\n";
    return errno;
  }

  /* Step 1: Fork process into sandbox runtime that links to the library */
  int sandbox_fd = fork();
  if (sandbox_fd == -1) {
    std::cerr << "Error: sandbox fork failed\n";
    return errno;
  } else if (!sandbox_fd) {
    /* Step 2: Run the vmsetup code from simple.cpp and fix bugs as needed */
    sandbox_run(to_sandbox_fd, from_sandbox_fd);
  }

  uint8_t *to_sm = (uint8_t *)mmap(NULL, 1024 * 4, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, to_sandbox_fd, 0);
  uint8_t *from_sm = (uint8_t *)mmap(NULL, 1024 * 4, PROT_READ, MAP_SHARED,
                                     from_sandbox_fd, 0);

  struct sandbox sandbox = {.fd = sandbox_fd, .to_mem = to_sm, .from_mem = from_sm};

  // create a buffer for input bytes
  char *input_stream = new char[100];
  if (!input_stream) {
    std::cerr << "Error: " << PROGRAM_STATUS_MSG[MEMORY_ALLOC_ERR_MSG] << "\n";
    return 1;
  }

  // Read bytes from an image file into input_stream
  get_image_bytes(input_stream);

  // Parse header of the image to get its dimensions
  ImageHeader *header = sb_parse_image_header(&sandbox, input_stream);

  if (header->status_code != HEADER_PARSING_STATUS_SUCCEEDED) {
    std::cerr << "Error: " << PROGRAM_STATUS_MSG[header->status_code] << "\n";
    return 1;
  }

  char *output_stream = new char[header->height * header->width];
  if (!output_stream) {
    std::cerr << "Error: " << PROGRAM_STATUS_MSG[MEMORY_ALLOC_ERR_MSG] << "\n";
    return 1;
  }

  sb_parse_image_body(&sandbox, input_stream, header, image_parsing_progress,
                      output_stream);

  std::cout << "Image pixels: " << std::endl;
  for (unsigned int i = 0; i < header->height; i++) {
    for (unsigned int j = 0; j < header->width; j++) {
      unsigned int index = i * header->width + j;
      std::cout << (unsigned int)output_stream[index] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "\n";

  free(header);
  delete[] input_stream;
  delete[] output_stream;

  return 0;
}
