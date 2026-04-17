shim is like a wrapper

trampolines and springboards

again figure out how to set up shared memory

Read this: https://hv.smallkirby.com/en/bootloader/cleanup_memmap
Want to figure out mmap a certain amount of mem RW access and map
the same physical pages onto the hypervisor's memory

fwizzle 
* basically turn the pointers into a way that the vm can understand
* if in the host you allocate a gig, 
and you want to give it to a vm, then the virtual addrs used
are not identical (convert ptr to terms ) 
* RLBox has a handler for this 

triaging
* try to find smallest point of change
* HOW TO SET UP SHARED MEMORY

4/9/26
file path based sharing will work

TODO:
figure out what tinykvm filtering is doing
* maybe kvm is limiting things

4/17/26
* Figure out a way to control what address the vm assigns memory to  
* Modify tainted volatile to copy from guest
* Setup the stack inside the vm to be the same stack depth as the host
* Quick suggesstion: copy 1K of the stack
* Look to RLBox plugin;  
* Before you start executing instructions in a funciton; mod 32 == 8
  (look up why this is)
* Change calls to enter vm and do the necessary things (since we can't
call functions directly)

* Order of operations
  * Get a simple example of passing a buffer, read the contents, dereference
  the pointer 
  * Try using the separate stack invoker on the code alone
  * Custom DLMALLOC
  * Then go to the RLBox plugin part
* Minor things
  * Convert process based implementation to a thread
* Gradual things
  * Read up on mmap and all the neat things you can do with
  this (e.g lazy page allocation)

* Custom DLMALLOC
  * Mode 1: sbrk mode
    * Watermark as your memory gets filled to sort of mark the highest
    possible  
  * Mode 2: mmap mode
    * ask OS for pages and it will give you a contiguous range
    * want to base the mapping off of the allocated shared memory 
    * keep track of all memory you have given dlmalloc
* DLMALLOC can be exposed to acts as malloc and free
