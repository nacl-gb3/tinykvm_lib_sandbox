#include "sandbox.hpp"
#include "assert.hpp"
#include "load_file.hpp"
#include "sandbox.hpp"
#include "shm_lib.h"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <tinykvm/machine.hpp>

#include <tinykvm/rsp_client.hpp>

#define GUEST_MEMORY 0x80000000             /* 2GB memory */
#define GUEST_WORK_MEM 1024UL * 1024 * 1024 /* MB working mem */

static uint64_t verify_exists(tinykvm::Machine &vm, const char *name) {
  uint64_t addr = vm.address_of(name);
  if (addr == 0x0) {
    //		fprintf(stderr, "Error: '%s' is missing\n", name);
    //		exit(1);
  }
  return addr;
}

inline timespec time_now();
inline long nanodiff(timespec start_time, timespec end_time);

struct shmbuf shm;
std::atomic_uint64_t shm_ref_cnt;

int shm_init(size_t im_page_cnt) {
  shm.sem1 = (sem_t *)malloc(sizeof(sem_t));
  shm.sem2 = (sem_t *)malloc(sizeof(sem_t));

  if (sem_init(shm.sem1, 1, 0) == -1) {
    fprintf(stderr, "sem1 init failed\n");
    return -1;
  }
  if (sem_init(shm.sem2, 1, 0) == -1) {
    fprintf(stderr, "sem2 init failed\n");
    return -1;
  }

  shm.fd = open("/tmp/shm", O_RDWR | O_CREAT, S_IRWXU);
  if (shm.fd == -1) {
    fprintf(stderr, "open for shared memory failed\n");
    return -1;
  }

  shm.im.size = im_page_cnt * BASE_INTERMEM_SIZE;

  if (ftruncate(shm.fd, shm.im.size) == -1) {
    fprintf(stderr, "ftruncate for shm fd failed\n");
    return -1;
  }

  shm.im.buf = (uint8_t *)mmap(NULL, shm.im.size, PROT_READ | PROT_WRITE,
                               MAP_SHARED, shm.fd, 0);

  printf("0x%lx\n", (size_t)shm.sem1);
  shm_ref_cnt++;

  return (shm.im.buf == NULL);
}

int sandbox_run() {
  char prog[] = "./build/helloshm";
  std::vector<uint8_t> binary;
  std::vector<std::string> args;
  std::string filename = prog;
  binary = load_file(filename);

  const tinykvm::DynamicElf dyn_elf = tinykvm::is_dynamic_elf(
      std::string_view{(const char *)binary.data(), binary.size()});
  if (dyn_elf.is_dynamic) {
    // Add ld-linux.so.2 as first argument
    static const std::string ld_linux_so = "/lib64/ld-linux-x86-64.so.2";
    binary = load_file(ld_linux_so);
    args.push_back(ld_linux_so);
  }

  args.push_back(prog);
  args.push_back((char *)shm.im.size);

  tinykvm::Machine::init();

  tinykvm::Machine::install_unhandled_syscall_handler(
      [](tinykvm::vCPU &cpu, unsigned scall) {
        switch (scall) {
        case 0x10000:
          cpu.stop();
          break;
        case 0x10303: {
          printf("CALLING COPY FROM HOST: 0x%x\n", scall);
          auto regs = cpu.registers();
          uint64_t addr = (uint64_t)regs.rdi;
          size_t len = (size_t)regs.rsi;

          if (sem_wait(shm.sem1) == -1) {
            perror("sem_wait");
            regs.rax = errno;
          } else {
            // TODO: think of ways that things can go wrong and
            // handle them
            cpu.machine().copy_to_guest(addr, shm.im.buf, len);
            regs.rax = 0;
          }
          cpu.set_registers(regs);
          printf("FINISHED COPY FROM HOST: 0x%x\n", scall);
          break;
        }
        case 0x10505: {
          printf("CALLING COPY TO HOST: 0x%x\n", scall);
          auto regs = cpu.registers();
          uint64_t addr = (uint64_t)regs.rdi;
          size_t len = (size_t)regs.rsi;

          // TODO: think of ways that things can go wrong and
          // handle them
          cpu.machine().copy_from_guest(shm.im.buf, addr, len);

          if (sem_post(shm.sem2) == -1) {
            perror("sem_post");
            regs.rax = errno;
          } else {
            regs.rax = 0;
          }
          cpu.set_registers(regs);
          printf("FINISHED COPY TO HOST: 0x%x\n", scall);
          break;
        }
        default:
          printf("Unhandled system call: %u\n", scall);
          auto regs = cpu.registers();
          regs.rax = -ENOSYS;
          cpu.set_registers(regs);
        }
      });

  const std::vector<tinykvm::VirtualRemapping> remappings{{
      .phys = 0x0,
      .virt = 0xC000000000,
      .size = 512ULL << 20,
  }};

  /* Setup */
  const tinykvm::MachineOptions options{
      .max_mem = GUEST_MEMORY,
      .max_cow_mem = GUEST_WORK_MEM,
      .reset_free_work_mem = 0,
      .vmem_base_address =
          uint64_t(getenv("UPPER") != nullptr ? 0x40000000 : 0x0),
      .remappings{remappings},
      .verbose_loader = true,
      .hugepages = (getenv("HUGE") != nullptr),
      .relocate_fixed_mmap = (getenv("GO") == nullptr),
      .executable_heap = dyn_elf.is_dynamic,
  };

  tinykvm::Machine master_vm{binary, options};
  // master_vm.print_pagetables();
  if (dyn_elf.is_dynamic) {
    // TODO: figure out how to automate this
    // allow reads from following paths
    static const std::vector<std::string> allowed_readable_paths({
        prog,
        "/",
        ".",

        // process information
        //"/proc/self/exe", // causes SIGSEGV when uncommented
        "/proc/self/cmdline",
        "/proc/self/environ",
        //"/proc/self/fd/4096", // causes SIGSEGV when uncommented

        "/etc/ld.so.preload",
        "/etc/ld.so.cache",

        // Add all common standard libraries to the list of allowed readable
        // paths
        "/lib64/ld-linux-x86-64.so.2",
        "/lib64/libgcc_s.so.1",
        "/lib64/libc.so.6",
        "/lib64/libm.so.6",
        "/lib64/libpthread.so.0",
        "/lib64/libdl.so.2",
        "/lib64/libstdc++.so.6",
        "/lib64/librt.so.1",
        "/lib64/libz.so.1",
        "/lib64/libexpat.so.1",

        "/lib64/glibc-hwcaps/x86-64-v2/",
        "/lib64/glibc-hwcaps/x86-64-v3/",
        "/lib64/glibc-hwcaps/x86-64-v4/",

        "/lib64/glibc-hwcaps/x86-64-v2/libc.so.6",
        "/lib64/glibc-hwcaps/x86-64-v3/libc.so.6",
        "/lib64/glibc-hwcaps/x86-64-v4/libc.so.6",
        "/lib64/glibc-hwcaps/x86-64-v2/libstdc++.so.6",
        "/lib64/glibc-hwcaps/x86-64-v3/libstdc++.so.6",
        "/lib64/glibc-hwcaps/x86-64-v4/libstdc++.so.6",

        // SELinux Compat
        "/lib64/libselinux.so.1",
        "/lib64/glibc-hwcaps/x86-64-v2/libselinux.so.1",
        "/lib64/glibc-hwcaps/x86-64-v3/libselinux.so.1",
        "/lib64/glibc-hwcaps/x86-64-v4/libselinux.so.1",

    });
    master_vm.fds().set_open_readable_callback([&](std::string &path) -> bool {
      return std::find(allowed_readable_paths.begin(),
                       allowed_readable_paths.end(),
                       path) != allowed_readable_paths.end();
    });

    // allow writes to shared memory
    // static const std::vector<std::string> allowed_writable_paths(
    //     {abs_shmpath, "/tmp/test", "README.md"});
    // master_vm.fds().set_open_writable_callback([&](std::string &path) -> bool
    // {
    //   return std::find(allowed_writable_paths.begin(),
    //                    allowed_writable_paths.end(),
    //                    path) != allowed_writable_paths.end();
    // });
  }

  // for debugging information
  // master_vm.set_verbose_system_calls(true);

  master_vm.setup_linux(args, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  const auto rsp = master_vm.stack_address();

  uint64_t call_addr = verify_exists(master_vm, "my_backend");

  asm("" ::: "memory");
  auto t0 = time_now();
  asm("" ::: "memory");

  /* Normal execution of _start -> main() */
  try {
    master_vm.run();
  } catch (const tinykvm::MachineException &me) {
    master_vm.print_registers();
    fprintf(stderr, "Machine exception: %s  Data: 0x%lX\n", me.what(),
            me.data());
    throw;
  } catch (...) {
    master_vm.print_registers();
    throw;
  }

  asm("" ::: "memory");
  auto t1 = time_now();
  asm("" ::: "memory");

  if (call_addr == 0x0) {
    double t = nanodiff(t0, t1) / 1e9;
    printf("Time: %fs Return value: %ld\n", t, master_vm.return_value());
  }

  return master_vm.return_value();
}

void get_shm_obj(struct shmbuf *nshm) {
  nshm->sem1 = shm.sem1;
  nshm->sem2 = shm.sem2;
  nshm->fd = shm.fd;
  nshm->im = {
      .buf = (uint8_t *)mmap(NULL, shm.im.size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, shm.fd, 0),

      .size = shm.im.size,
  };

  printf("0x%lx\n", (size_t)nshm->im.buf);

  shm_ref_cnt++;
}

int shm_obj_free(struct shmbuf *fshm) {
  int res = munmap(fshm->im.buf, fshm->im.size);
  shm_ref_cnt--;
  if (shm_ref_cnt == 0) {
    free(shm.sem1);
    free(shm.sem2);
  }
  return res;
}

timespec time_now() {
  timespec t;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
  return t;
}

long nanodiff(timespec start_time, timespec end_time) {
  return (end_time.tv_sec - start_time.tv_sec) * (long)1e9 +
         (end_time.tv_nsec - start_time.tv_nsec);
}
