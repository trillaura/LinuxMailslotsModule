# Linux Mailslots Module

This is a module for Linux Kernel 4.13 implementing a special device file that is accessible according to *FIFO* style semantic, posting or delivering data units atomically and in data separation, and that is configurable via *ioctl* interface.

This device is called **mailslot** as the service offered by Windows, which this project is inspired to.
The complete description is available in the [documentation].

# Features
- **First In First Out** semanthic for read and write operations
- **Atomic** data unit read/write (*all or nothing*)
- **Sinchronized** concurrent access at each mailslot instance
- Run-time operations' policy configuration (**blocking/non-blocking behavior**) 
- Run-time **maximum mailslot size** configuration (limited by a compile-time *upper limit* definition)
- Run-time **maximum message size** configuration
- Compile-time **maximum mailslot instances** definition
- Compile-time devices' **minor number's range** definition

More details are available in the [documentation].
        
# Why not *mkfifo*?
A mailslot device manages *independent and atomic* data units.
A standard mkfifo device allows reading bytes among *multiple* reads.
Performance comparison is discussed in the [documentation].

# Build
```sh
$ cd module
/module$ make
```
# Load/unload module
After the module was compiled, it can be loaded dynamically into the kernel using command specified in the Makefile:
```sh
/module$ sudo make load
```
If loading is successful, the mailslot device will appear among *proc* devices. 
Check it with:
```sh
$ more /proc/devices | grep mailslot
```
The output will show the major number which was assigned by the kernel to the driver.
Another way to check the major number is using:
```sh
$ dmesg
```
that shows kernel message console.

It can be unloaded with:
```sh
/module$ sudo make unload
```

If unloading is successful, the mailslot device will appear no more among *proc* devices. 
Check it with:
```sh
$ more /proc/devices | grep mailslot
```

# Usage example
An example of use is implemented in the directory *user/* and can be exaceuted running:
```sh
$ cd user
/user$ sudo ./user <name> <major> <minor>
```
where name is the name the user wants to assign to the device file, major is the major number read from *dmesg* output and minor is the minor number the user wants to assign to the device file instance.
If the minor is out of the range for minor numbers defined in the module, the creation of the instance will fail.

License
----

GNU

[documentation]: <https://github.com/trillaura/LinuxMailslotsModule/blob/master/documentation/Linux_mailslot.pdf>
