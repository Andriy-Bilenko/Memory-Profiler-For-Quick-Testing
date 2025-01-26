# Memory-Profiler-For-Quick-Testing-
lightweight malloc/free/calloc/realloc profiler written in C


# Memory Profiler for Quick Testing  
*A lightweight malloc/free/calloc/realloc profiler written in C*

## 🚀 Overview  
This project provides a lightweight memory profiling tool designed for quick testing and debugging. It helps you track memory allocations and deallocations (`malloc`, `calloc`, `realloc`, and `free`) to identify memory leaks or unexpected heap usage in your code.

The profiler is **written in C**, making it compatible with both C and C++ projects. It was specifically created as a utility to debug scenarios where C++ code used `malloc` instead of `new`, and overloading `new` was ineffective. Since `malloc`-style memory functions are universal, this profiler works 100% of the time.

> **Note**: This utility is intended for **tinkering and debugging purposes only**, not for production use. It introduces some overhead and overrides the default memory functions, which might not align with production requirements.

---

## 📂 Features  
- **Universal Tracking**: Monitors `malloc`, `calloc`, `realloc`, and `free` usage.  
- **Minimal Overhead**: Designed for debugging without heavy profiling tools.  
- **Simple Integration**: Just include `memprof_c.c` and `memprof_c.h` in your project.  
- **Ease of Use**: Plug it into your project, include the header, and track memory allocations in a few lines of code (see `example.cpp` and documentation in the header file).  
- **C++ Friendly**: Works seamlessly with C++ code where standard memory allocation operators (`new`, `delete`) seemingly aren't used.  

---

## ⚠️ Limitations

- **Not Suitable for Production**: The profiler adds overhead and overrides core memory allocation functions.
- **Limited Tracking**: Only tracks up to a predefined number of allocations (`MAX_ALLOCATIONS_TRACKING`).
- **Non-Thread-Safe**: Current implementation does not handle multi-threaded environments.




If you ran into some issues by any chance or need to contact the developer, it would be great to recieve your valuable feedback on email: *bilenko.a.uni@gmail.com*.

<div align="right">
<table><td>
<a href="#start-of-content">↥ Scroll to top</a>
</td></table>
</div>
