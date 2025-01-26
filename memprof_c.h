/******************************************************************************
 * Project:     Memory Profiler (For Quick Testing)
 * File:        memprof_c.h
 * Description: A simple malloc/free/calloc/realloc profiler for quick testing.
 *              This header file allows tracking memory allocations and
 *              deallocations with minimal setup, perfect for testing
 *              without requiring large, complex profiling tools.
 *
 * Copyright (c) 2025 Andriy Bilenko. All rights reserved.
 *
 * Licensed under the GPLv3 License. You may obtain a copy of the License at:
 *
 *     https://www.gnu.org/licenses/gpl-3.0.html
 *
 * @file        memprof_c.h
 * @version     1.0
 * @date        2025-01-26
 * @author      Andriy Bilenko
 *
 * This header file provides basic memory tracking functionality to quickly
 * test memory usage and detect potential leaks by integrating it into your
 *project.
 *
 * Usage:
 * 1. Include this header file in your project (include into C or C++ file).
 * 2. Call `enableMemoryLogging(true)` at the start of a part of your interest.
 * 3. Call `printMemoryUsage()` to view memory statistics upon request.
 * 4. Call `profilerReset()` to restart / clean-up the profiler.
 ******************************************************************************/

#ifndef MEMPROF_C_H
#define MEMPROF_C_H

#include <dlfcn.h>
#include <stdbool.h>
#include <string.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
 * @brief MAX_ALLOCATIONS_TRACKING
 * change if you want more/less memory tracking
 */
const size_t MAX_ALLOCATIONS_TRACKING = 1000;

struct AllocationInfo {
	void* pointer;
	size_t size;
};

extern AllocationInfo allocations[MAX_ALLOCATIONS_TRACKING];

// Overriding function declarations
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t newSize);

// Utility functions (use only them)
/**
 * @brief enableMemoryLogging()
 * enables memory events (malloc, free, calloc, realloc) logging to sdterr
 * stream (how many bytes and at which address were allocated/freed)
 *
 * @param enable if set to true starts logging and saving all allocations in
 * `allocations` array and logging to sdterr upon memory events.
 * if set to false stops logging to `allocations` and stderr, does not clean
 * itself. If there are more allocations than MAX_ALLOCATIONS_TRACKING, then it
 * will notify you that "Total freed" and "Currently allocated" it shows are
 * considered to be no longer valid ("Total allocated" will still be valid)
 */
void enableMemoryLogging(bool enable);
/**
 * @brief printMemoryUsage()
 * prints to stderr "Total allocated", "Total freed", "Currently allocated" in
 * bytes. Additionally lets you know if number of allocations is greater than
 * MAX_ALLOCATIONS_TRACKING
 */
void printMemoryUsage();

/**
 * @brief profilerReset()
 * resets / cleans up the state of a profiler (all values including
 * `allocations` array, enableMemoryLogging, totalAllocated, totalFreed,
 * allocationCount, is_allocations_array_overflow are set to 0's)
 */
void profilerReset();

#endif	// MEMPROF_C_H
