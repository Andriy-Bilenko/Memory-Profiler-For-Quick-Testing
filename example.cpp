#include <cxxabi.h>	 // for demangling typeid

#include <iostream>
#include <memory>
#include <vector>

#include "memprof_c.h"

void useVector() {
	std::vector<int> vec;
	for (int i = 0; i < 10; ++i) {
		vec.push_back(i * 2);
	}
}

int main() {
	// Test1
	std::cout << "TEST1: start.\r\n";
	enableMemoryLogging(true);
	printMemoryUsage();

	int* arr = (int*)malloc(10 * sizeof(int));
	arr = (int*)realloc(arr, 20 * sizeof(int));

	printMemoryUsage();
	free(arr);
	arr = nullptr;
	printMemoryUsage();

	void* ptr1 = malloc(40);
	void* ptr2 = calloc(5, 20);
	ptr1 = realloc(ptr1, 80);
	free(ptr1);
	ptr1 = nullptr;
	free(ptr2);
	ptr2 = nullptr;

	printMemoryUsage();
	profilerReset();
	std::cout << "TEST1: end.\r\n";

	// Test2
	std::cout << "TEST2: start.\r\n";
	enableMemoryLogging(true);
	useVector();
	enableMemoryLogging(false);
	printMemoryUsage();
	profilerReset();
	std::cout << "TEST2: end.\r\n";

	// Test3
	std::cout << "TEST3: start.\r\n";
	enableMemoryLogging(true);
	std::string s1 = "Hello, world!";
	s1 += " More data to trigger allocation.";
	{
		printMemoryUsage();
		auto sp = std::make_shared<int>(42);
		printMemoryUsage();
	}
	printMemoryUsage();
	profilerReset();
	std::cout << "TEST3: end.\r\n";

	// Test4 (the way I used it to test how this feature works)
	std::cout << "TEST4: start.\r\n";
	std::string s2 = "another string";
	enableMemoryLogging(true);
	const char* cstr = s2.c_str();
	const char* mangled_name = typeid(cstr).name();
	std::cout << "Mangled name: " << typeid(cstr).name() << std::endl;
	char* demangled_name =
		abi::__cxa_demangle(typeid(cstr).name(), 0, 0, nullptr);
	std::cout << "Demangled name: " << demangled_name << std::endl;
	free(demangled_name);  // appears that demangled_name has to be freed
	printMemoryUsage();
	std::cout << "TEST4: end.\r\n";

	return 0;
}