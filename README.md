# C=
After the great success of the C programming language, it is time for a sequal: C=

This purely interpreted programming language improves upon a few things that C didn't get quite right:

**Type safety**
As we all probably know, C is not type-safe. This can get you in a lot of trouble. For this reason, C= guarantuees absolute type safety! This is done through the innovative process of only having one type. Everything is an integer. Always. This means no more writing 'uint64_t' everywhere. Every variable is an integer, every function returns an integer, every pointer is an integer. Nobody liked floating point variables anyway.

**Preprocessor**
The C preprocossor is outdated... Header guards? Who thought that was a good idea? In C=, the preprocessor has been removed. No importing other files, no weird recursive function-like macros. Simplicity is key.

**Build system**
I think we can all agree one of the greatest shortcomings of C is the wild west that is building your applications. Make, CMake, Ninja... For this reason, C= is a purely interpreted language. Just write your code, run the interpreter, and you're on your way.

## Building
This software has been developed on Pop!_OS \[version\]. The Windows build has also been (briefly) tested on WSL with Ubuntu 22.04 LTS.

Building might work on ealier or later versions of any software versions mentioned here or on other Debian-based systems. Maybe even other Linux distributions. These are just the versions I used.

### Pop!_OS
gcc version: 
make version:
ar version:

### WSL Ubuntu 22.04
gcc version: 11.2.0
make version: 4.3
ar version: 2.38

### Building for Linux
```bash
$ make [release]
```

### Building for Windows
Install the mingw-w64 cross compiler suite if you haven't already. I used version X.
```bash
$ sudo apt install mingw-w64
```

```bash
$ ./build_windows.sh [release]
```