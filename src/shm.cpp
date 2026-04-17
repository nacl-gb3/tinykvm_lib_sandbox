#include "sandbox.hpp"
#include "shm_lib.h"
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const char *PROGRAM_STATUS_MSG[] = {"Succeeded", "Invalid image",
                                           "Incomplete Image",
                                           "Memory allocation failure"};

#define MEMORY_ALLOC_ERR_MSG 3

/* Step 4: Create a manual wrapper around call to lib.h */
ImageHeader *sb_parse_image_header(struct sandbox *sb, char *in) {
  // write argument to memory
  return nullptr;
}

void sb_parse_image_body(struct sandbox *sb, char *in, ImageHeader *header,
                         OnProgress *on_progress, char *out) {
  // parse_image_body(in, header, on_progress, out);
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
  int err = 0;
  int fd;
  struct shmbuf *shmp;

  std::string shmpath = "/sbshm";

  char str[6] = "hello";
  size_t len = 6;

  err = shm_init();
  if (err) {
    perror("shm_init");
    return err;
  }

  int sandbox_fd = fork();
  if (sandbox_fd == -1) {
    std::cerr << "Error: sandbox fork failed\n";
    return errno;
  } else if (!sandbox_fd) {
    /* Step 2: Run the vmsetup code from simple.cpp and fix bugs as needed */
    err = sandbox_run();
    if (err) {
      perror("hello_shm");
    }
    return err;
  }
  std::cout << "successful fork\n";

  shmp = get_shm_obj();

  std::cout << "attempting to strlcpy\n";
  memcpy(shmp->im.buf, str, len);

  // pass control to the sandbox runtime
  std::cout << "passing control to sandbox\n";
  if (sem_post(&shmp->sem1) == -1) {
    perror("sem_post to_guest");
    return -1;
  }

  // return control to program
  if (sem_wait(&shmp->sem2) == -1) {
    perror("sem_wait from_guest");
    return -1;
  }

  std::cout << "control returned from sandbox\n";

  std::cout << (char *)shmp->im.buf << "\n";

  // print shared result

  /*
  // create a buffer for input bytes
  char *input_stream = new char[100];
  if (!input_stream) {
    std::cerr << "Error: " << PROGRAM_STATUS_MSG[MEMORY_ALLOC_ERR_MSG] << "\n";
    return 1;
  }

  // Read bytes from an image file into input_stream
  get_image_bytes(input_stream);

  // Parse header of the image to get its dimensions
  ImageHeader *header = sb_parse_image_header(NULL, input_stream);

  if (header->status_code != HEADER_PARSING_STATUS_SUCCEEDED) {
    std::cerr << "Error: " << PROGRAM_STATUS_MSG[header->status_code] << "\n";
    return 1;
  }

  char *output_stream = new char[header->height * header->width];
  if (!output_stream) {
    std::cerr << "Error: " << PROGRAM_STATUS_MSG[MEMORY_ALLOC_ERR_MSG] << "\n";
    return 1;
  }

  sb_parse_image_body(NULL, input_stream, header, image_parsing_progress,
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
  */

  err = shm_uninit();

  return err;
}
