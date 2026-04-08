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
