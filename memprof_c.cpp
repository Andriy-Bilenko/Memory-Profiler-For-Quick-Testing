// memprof_c.cpp
#include "memprof_c.hpp"

// Real function pointers for overriding malloc, free, realloc, calloc
static void* (*real_malloc)(size_t) = nullptr;
static void (*real_free)(void*) = nullptr;
static void* (*real_realloc)(void*, size_t) = nullptr;
static void* (*real_calloc)(size_t, size_t) = nullptr;

static bool isMemLog = false;  // Global flag for logging memory usage
static size_t totalAllocated = 0;
static size_t totalFreed = 0;
static AllocationInfo allocations[MAX_ALLOCATIONS_TRACKING] = {};
static size_t allocationCount = 0;
static bool isAllocationsArrayOverflow = false;
static std::mutex allocationsMut;
static std::mutex loggingMut;

// for debugging
static void printAllocations() {
	allocationsMut.lock();
	loggingMut.lock();
	printf("---- Tracked Allocations: %zu\n", allocationCount);
	printf("---------------------\n");
	for (size_t i = 0; i < MAX_ALLOCATIONS_TRACKING; ++i) {
		if (allocations[i].pointer != NULL) {  // Only print valid entries
			printf("---- Index: %zu, Address: %p, Size: %zu bytes\n", i,
				   allocations[i].pointer, allocations[i].size);
		}
	}
	printf("---------------------\n");
	allocationsMut.unlock();
	loggingMut.unlock();
}

// initialize profiler memory functions
static int init_real_functions() {
	int ret = 0;
	if (!real_malloc) {
		real_malloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
		if (!real_malloc) {
			loggingMut.lock();
			fprintf(stderr, "ERROR loading `malloc`\n");
			loggingMut.unlock();
			ret = -1;
		}
	}
	if (!real_free) {
		real_free = (void (*)(void*))dlsym(RTLD_NEXT, "free");
		if (!real_free) {
			loggingMut.lock();
			fprintf(stderr, "ERROR loading `free`\n");
			loggingMut.unlock();
			ret = -1;
		}
	}
	if (!real_calloc) {
		real_calloc = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
		if (!real_calloc) {
			loggingMut.lock();
			fprintf(stderr, "ERROR loading `calloc`\n");
			loggingMut.unlock();
			ret = -1;
		}
	}
	if (!real_realloc) {
		real_realloc = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
		if (!real_realloc) {
			loggingMut.lock();
			fprintf(stderr, "ERROR loading `realloc`\n");
			loggingMut.unlock();
			ret = -1;
		}
	}
	return ret;
}

// Override malloc
void* malloc(size_t size) {
	init_real_functions();
	void* p = real_malloc(size);
	allocationsMut.lock();
	loggingMut.lock();
	if (isMemLog) {
		if (p) {
			if (allocationCount < MAX_ALLOCATIONS_TRACKING) {
				allocations[allocationCount++] = {p, size};
			} else {
				isAllocationsArrayOverflow = true;
			}
			if (isAllocationsArrayOverflow) {
				fprintf(stderr,
						"[Custom malloc] RAN OUT OF allocationCount. FROM NOW "
						"ON \"Total freed\" and \"Currently allocated\" ARE "
						"OUTDATED.\n");
			}
			totalAllocated += size;
			fprintf(stderr, "[Custom malloc] Allocated %zu bytes at %p\n", size,
					p);
		} else {
			fprintf(stderr,
					"[Custom malloc] FAILED Allocating %zu bytes at %p\n", size,
					p);
		}
	}
	allocationsMut.unlock();
	loggingMut.unlock();
	return p;
}

// Override free
void free(void* ptr) {
	init_real_functions();
	allocationsMut.lock();
	loggingMut.lock();
	if (isMemLog) {
		bool found_address_to_free = false;
		for (size_t i = 0; i < allocationCount; ++i) {
			if (allocations[i].pointer == ptr) {
				found_address_to_free = true;
				size_t freed_size = allocations[i].size;
				totalFreed += freed_size;
				fprintf(stderr, "[Custom free] Freed %zu bytes at %p\n",
						freed_size, ptr);
				allocations[i] = allocations[allocationCount - 1];
				--allocationCount;
				break;
			}
		}
		if (!found_address_to_free) {
			fprintf(stderr, "[Custom free] Freed unknown bytes at %p\n", ptr);
			// printAllocations(); // debugging
		}
	}
	allocationsMut.unlock();
	loggingMut.unlock();
	real_free(ptr);
}

// Override calloc
void* calloc(size_t num, size_t size) {
	init_real_functions();
	void* p = real_calloc(num, size);
	allocationsMut.lock();
	loggingMut.lock();
	if (isMemLog) {
		if (p) {
			if (allocationCount < MAX_ALLOCATIONS_TRACKING) {
				allocations[allocationCount++] = {p, num * size};
			} else {
				isAllocationsArrayOverflow = true;
			}
			if (isAllocationsArrayOverflow) {
				fprintf(stderr,
						"[Custom calloc] RAN OUT OF allocationCount. FROM NOW "
						"ON \"Total freed\" and \"Currently allocated\" ARE "
						"OUTDATED.\n");
			}
			totalAllocated += num * size;
			fprintf(stderr,
					"[Custom calloc] Allocated %zu bytes (array of %zu "
					"elements) "
					"at %p\n",
					num * size, size, p);
		} else {
			fprintf(stderr,
					"[Custom calloc] FAILED Allocating %zu bytes (array of %zu "
					"elements) "
					"at %p\n",
					num * size, size, p);
		}
	}
	allocationsMut.unlock();
	loggingMut.unlock();
	return p;
}

// Override realloc
void* realloc(void* ptr, size_t newSize) {
	init_real_functions();
	// if ptr is NULL works as malloc
	// if newSize is 0 works as free
	// else it is freeing ptr and mallocing newSize
	void* p = real_realloc(ptr, newSize);

	allocationsMut.lock();
	loggingMut.lock();
	if (isMemLog) {
		if (p) {
			if (!ptr) {
				// malloc
				if (allocationCount < MAX_ALLOCATIONS_TRACKING) {
					allocations[allocationCount++] = {p, newSize};
				} else {
					isAllocationsArrayOverflow = true;
				}
				totalAllocated += newSize;
			} else if (!newSize) {
				// free
				for (size_t i = 0; i < allocationCount; ++i) {
					if (allocations[i].pointer == ptr) {
						size_t freed_size = allocations[i].size;
						totalFreed += freed_size;
						allocations[i] = allocations[allocationCount - 1];
						--allocationCount;
						break;
					}
				}
			} else {
				// free and malloc
				// free
				for (size_t i = 0; i < allocationCount; ++i) {
					if (allocations[i].pointer == ptr) {
						size_t freed_size = allocations[i].size;
						totalFreed += freed_size;
						allocations[i] = allocations[allocationCount - 1];
						--allocationCount;
						break;
					}
				}
				// malloc
				if (allocationCount < MAX_ALLOCATIONS_TRACKING) {
					allocations[allocationCount++] = {p, newSize};
				} else {
					isAllocationsArrayOverflow = true;
				}
				totalAllocated += newSize;
			}
			if (isAllocationsArrayOverflow) {
				fprintf(stderr,
						"[Custom realloc] RAN OUT OF allocationCount. "
						"FROM NOW "
						"ON \"Total freed\" and \"Currently "
						"allocated\" ARE "
						"OUTDATED.\n");
			}
			fprintf(stderr,
					"[Custom realloc] Reallocated %zu bytes from %p to "
					"%p\n",
					newSize, ptr, p);
		} else {
			fprintf(stderr,
					"[Custom realloc] FAILED Reallocating %zu bytes from "
					"%p to %p\n",
					newSize, ptr, p);
		}
	}
	allocationsMut.unlock();
	loggingMut.unlock();

	return p;
}

// Enable logging
void enableMemoryLogging(bool enable) {
	allocationsMut.lock();
	isMemLog = enable;
	allocationsMut.unlock();
}

// Print memory usage statistics
void printMemoryUsage() {
	allocationsMut.lock();
	loggingMut.lock();
	fprintf(stderr, "****************************************\r\n");
	fprintf(stderr, "**** Total allocated: %zu bytes\n", totalAllocated);
	fprintf(stderr, "**** Total freed: %zu bytes\n", totalFreed);
	fprintf(stderr, "**** Currently allocated: %zu bytes\n",
			totalAllocated - totalFreed);
	if (allocationCount >= MAX_ALLOCATIONS_TRACKING) {
		fprintf(stderr,
				"**** IMPORTANT: number of allocations exceeded "
				"MAX_ALLOCATIONS_TRACKING:\n**** \"Total freed\" and "
				"\"Currently allocated\" is likely to be wrong.");
	}
	fprintf(stderr, "****************************************\r\n");
	allocationsMut.unlock();
	loggingMut.unlock();
}

// reset the profiler state
void profilerReset() {
	allocationsMut.lock();
	isMemLog = false;
	totalAllocated = 0;
	totalFreed = 0;
	allocationCount = 0;
	isAllocationsArrayOverflow = false;
	memset(allocations, 0, sizeof(allocations));
	allocationsMut.unlock();
}
