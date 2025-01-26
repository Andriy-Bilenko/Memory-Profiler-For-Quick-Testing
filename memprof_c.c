// memprof_c.c
#include "memprof_c.h"

// Real function pointers for overriding malloc, free, realloc, calloc
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;

bool is_mem_log = false;  // Global flag for logging memory usage
size_t totalAllocated = 0;
size_t totalFreed = 0;
struct AllocationInfo allocations[MAX_ALLOCATIONS_TRACKING] = {};
size_t allocationCount = 0;
bool is_allocations_array_overflow = false;

// for debugging
static void printAllocations() {
    printf("---- Tracked Allocations: %zu\n", allocationCount);
    printf("---------------------\n");
    for (size_t i = 0; i < MAX_ALLOCATIONS_TRACKING; ++i) {
        if (allocations[i].pointer != NULL) { // Only print valid entries
            printf("---- Index: %zu, Address: %p, Size: %zu bytes\n", i, allocations[i].pointer, allocations[i].size);
        }
    }
    printf("---------------------\n");
}

// Initialize real functions
static void init_real_functions() {
	if (!real_malloc) {
		real_malloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
		if (!real_malloc) {
			fprintf(stderr, "Error loading real malloc\n");
			_Exit(1);
		}
	}
	if (!real_free) {
		real_free = (void (*)(void*))dlsym(RTLD_NEXT, "free");
		if (!real_free) {
			fprintf(stderr, "Error loading real free\n");
			_Exit(1);
		}
	}
	if (!real_calloc) {
		real_calloc = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
		if (!real_calloc) {
			fprintf(stderr, "Error loading real calloc\n");
			_Exit(1);
		}
	}
	if (!real_realloc) {
		real_realloc = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
		if (!real_realloc) {
			fprintf(stderr, "Error loading real realloc\n");
			_Exit(1);
		}
	}
}

// Override malloc
void* malloc(size_t size) {
	init_real_functions();
	void* p = real_malloc(size);
	if (is_mem_log) {
		if (p) {
			if (allocationCount < MAX_ALLOCATIONS_TRACKING) {
				allocations[allocationCount++] = {p, size};
			} else {
				is_allocations_array_overflow = true;
			}
			if (is_allocations_array_overflow) {
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
	return p;
}

// Override free
void free(void* ptr) {
	init_real_functions();
	if (ptr) {
		if (is_mem_log) {
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
				fprintf(stderr, "[Custom free] Freed unknown bytes at %p\n",
						ptr);
						// printAllocations(); // debugging
			}
		}
		real_free(ptr);
	} else {
		if (is_mem_log) {
			fprintf(stderr,
					"[Custom free] Tried freeing NULL (we saved you)\n");
		} else {
			real_free(ptr);	 // not saving anyone
		}
	}
}

// Override calloc
void* calloc(size_t num, size_t size) {
	init_real_functions();
	void* p = real_calloc(num, size);
	if (is_mem_log) {
		if (p) {
			if (allocationCount < MAX_ALLOCATIONS_TRACKING) {
				allocations[allocationCount++] = {p, num * size};
			} else {
				is_allocations_array_overflow = true;
			}
			if (is_allocations_array_overflow) {
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
	return p;
}

// Override realloc
void* realloc(void* ptr, size_t newSize) {
	// if ptr is NULL works as malloc
	// if newSize is 0 works as free
	// else it is freeing ptr and mallocing newSize
	init_real_functions();

	void* p{nullptr};

	if (!(ptr || newSize)) {
		if (is_mem_log) {
			fprintf(stderr,
					"[Custom realloc] Tried Reallocating 0 at NULL (same as "
					"freeing NULL) (we saved you)\n");
		} else {
			p = real_realloc(ptr, newSize);	 // not saving anyone
		}
	} else {
		p = real_realloc(ptr, newSize);
		if (is_mem_log) {
			if (p) {
				if (!ptr) {
					// malloc
					if (allocationCount < MAX_ALLOCATIONS_TRACKING) {
						allocations[allocationCount++] = {p, newSize};
					} else {
						is_allocations_array_overflow = true;
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
						is_allocations_array_overflow = true;
					}
					totalAllocated += newSize;
				}
				if (is_allocations_array_overflow) {
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
	}

	return p;
}

// Enable logging
void enableMemoryLogging(bool enable) { is_mem_log = enable; }

// Print memory usage statistics
void printMemoryUsage() {
	// I have no idea why, but using std::cout here leaked 1024 bytes
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
}

// reset the profiler state
void profilerReset() {
	is_mem_log = false;
	totalAllocated = 0;
	totalFreed = 0;
	allocationCount = 0;
	is_allocations_array_overflow = false;
	memset(allocations, 0, sizeof(allocations));
}
