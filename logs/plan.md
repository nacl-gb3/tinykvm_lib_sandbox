
## TinyKVM sandboxing module
* sandboxing mini hv that claims to run processes at native speeds
  * Seems to be true for hello and img_app, but img_app solution did not quite reach
  native speeds; may be better for more compuational workloads
* dealing with file permissions is annoying
  * i can't access process info without getting a segfault (which unfortunately
  means that I can't use this on binaries that use the sanitize flag); can we
  try emulating reading this file somehow perhaps; try reaching out on GitHub to
  project maintainer and in the meantime figure out workaround myself
  * not 100% sure, but may need to individually give permissions to each dynamically loaded
  library; no recursive permissions, but can potentially find a workaround in my own fork 

### What I want to accomplish with this
* Over this week: make a toy program (based on the RLBox example) that manually implements wrappers
for library functions that turn kvm on and access the vm to run the code 
  * figure out if this spawns a new process or not 
* **Figure out how to share memory**
  * Reading memory contents of virtual machine
* Basically do map with commands to do onto shared memory

* How to do benchmarks
  * Do a loop of about 100 computational tasks (ie no i/o)
  * disable frq scaling and pin the frq to something low (2.2Ghz);  
    * check kvm/tinykvm and see how it handles cpus
  * disable hyperthreading
  * timer should never measure more than ms (otherwise, timer itself can cause noise)
  * take entire benchmark, discard it, (warm-up) and run it again and save the result
  * run it 100 times and take the median


## Design

* oops
