#include "sandbox.hpp"
#include "assert.hpp"
#include "load_file.hpp"
#include "sandbox.hpp"
#include "shm_lib.h"
#include <algorithm>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
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

int sandbox_run(int to_fd, int from_fd) {
  uint8_t *from_sm = (uint8_t *)mmap(NULL, 1024 * 4, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, from_fd, 0);
  uint8_t *to_sm =
      (uint8_t *)mmap(NULL, 1024 * 4, PROT_READ, MAP_SHARED, to_fd, 0);

  std::vector<uint8_t> binary;
  std::vector<std::string> args;
  std::string filename = "/path/to/sandbox/so";
  binary = load_file(filename);

  const tinykvm::DynamicElf dyn_elf = tinykvm::is_dynamic_elf(
      std::string_view{(const char *)binary.data(), binary.size()});
  if (dyn_elf.is_dynamic) {
    // Add ld-linux.so.2 as first argument
    static const std::string ld_linux_so = "/lib64/ld-linux-x86-64.so.2";
    binary = load_file(ld_linux_so);
    args.push_back(ld_linux_so);
  }

  args.push_back("/path/to/sandbox/so");

  tinykvm::Machine::init();

  tinykvm::Machine::install_unhandled_syscall_handler(
      [](tinykvm::vCPU &cpu, unsigned scall) {
        switch (scall) {
        case 0x10000:
          cpu.stop();
          break;
        case 0x10001:
          throw "Unimplemented";
        case 0x10707:
          throw "Unimplemented";
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
    static const std::vector<std::string> allowed_readable_paths({
        "/path/to/sandbox/so",
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
  }

  master_vm.setup_linux(args, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  const auto rsp = master_vm.stack_address();

  uint64_t call_addr = verify_exists(master_vm, "my_backend");

  // /* Remote debugger session */
  // if (getenv("DEBUG")) {
  //   auto *vm = &master_vm;
  //   tinykvm::tinykvm_x86regs regs;

  //   if (getenv("VMCALL")) {
  //     master_vm.run();
  //   }
  //   if (getenv("FORK")) {
  //     master_vm.prepare_copy_on_write();
  //     vm = new tinykvm::Machine{master_vm, options};
  //     vm->setup_call(regs, call_addr, rsp);
  //     vm->set_registers(regs);
  //   } else if (getenv("VMCALL")) {
  //     master_vm.setup_call(regs, call_addr, rsp);
  //     master_vm.set_registers(regs);
  //   }

  //   tinykvm::RSP server{filename, *vm, 2159};
  //   printf("Waiting for connection localhost:2159...\n");
  //   auto client = server.accept();
  //   if (client != nullptr) {
  //     /* Debugging session of _start -> main() */
  //     printf("Connected\n");
  //     try {
  //       // client->set_verbose(true);
  //       while (client->process_one())
  //         ;
  //     } catch (const tinykvm::MachineException &e) {
  //       printf("EXCEPTION %s: %lu\n", e.what(), e.data());
  //       vm->print_registers();
  //     }
  //   } else {
  //     /* Resume execution normally */
  //     vm->run();
  //   }
  //   /* Exit after debugging */
  //   return 0;
  // }

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
