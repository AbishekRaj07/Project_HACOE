#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <string>
#include <limits>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <cpuid.h>

using namespace std;

void printConstraint(const string& category, const string& name, const string& value) {
    cout << left << setw(15) << category << " | " 
         << left << setw(30) << name << " | " 
         << value << "\n";
}

void checkCompilerConstraints() {
    cout << "\n=== COMPILER & LANGUAGE CONSTRAINTS ===\n";
    string cpp_std;
    if (__cplusplus == 202002L) cpp_std = "C++20";
    else if (__cplusplus == 201703L) cpp_std = "C++17";
    else if (__cplusplus == 201402L) cpp_std = "C++14";
    else cpp_std = to_string(__cplusplus);
    
    printConstraint("Language", "C++ Standard", cpp_std);
    printConstraint("Compiler", "Architecture", sizeof(void*) == 8 ? "64-bit" : "32-bit");
    
#ifdef __clang__
    printConstraint("Compiler", "Clang Version", __clang_version__);
#elif defined(__GNUC__)
    printConstraint("Compiler", "GCC Version", to_string(__GNUC__) + "." + to_string(__GNUC_MINOR__));
#endif

#ifdef __OPTIMIZE__
    printConstraint("Compiler", "Optimization Level", "Optimized Build");
#else
    printConstraint("Compiler", "Optimization Level", "Unoptimized (-O0)");
#endif
}

void checkOSConstraints() {
    cout << "\n=== OPERATING SYSTEM CONSTRAINTS ===\n";
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
        printConstraint("OS Limit", "Max Open Files", to_string(rl.rlim_cur));
    if (getrlimit(RLIMIT_STACK, &rl) == 0) {
        if (rl.rlim_cur == RLIM_INFINITY) printConstraint("OS Limit", "Stack Size", "Unlimited");
        else printConstraint("OS Limit", "Stack Size", to_string(rl.rlim_cur / 1024) + " KB");
    }
    if (getrlimit(RLIMIT_NPROC, &rl) == 0)
        printConstraint("OS Limit", "Max User Threads", to_string(rl.rlim_cur));
    printConstraint("OS Limit", "Virtual Memory Page Size", to_string(sysconf(_SC_PAGESIZE)) + " Bytes");
}

void checkHardwareMemoryConstraints() {
    cout << "\n=== HARDWARE & MEMORY CONSTRAINTS ===\n";
    printConstraint("Hardware", "Logical CPU Cores", to_string(thread::hardware_concurrency()));
#ifdef __cpp_lib_hardware_interference_size
    printConstraint("Hardware", "Cache Line Size", to_string(std::hardware_constructive_interference_size) + " Bytes");
#else
    printConstraint("Hardware", "Cache Line Size", "64 Bytes (Assumed)");
#endif
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) == 0) {
        long long totalRAM = memInfo.totalram * memInfo.mem_unit;
        long long freeRAM = memInfo.freeram * memInfo.mem_unit;
        printConstraint("Memory", "Total Physical RAM", to_string(totalRAM / (1024 * 1024)) + " MB");
        printConstraint("Memory", "Available Free RAM", to_string(freeRAM / (1024 * 1024)) + " MB");
    }
}

void checkCPUSIMDConstraints() {
    cout << "\n=== CPU MICROARCHITECTURE (SIMD) ===\n";
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        printConstraint("CPU/SIMD", "AVX (256-bit) Support", (ecx & bit_AVX) != 0 ? "Yes" : "No");
        printConstraint("CPU/SIMD", "SSE4.1 (128-bit) Support", (ecx & bit_SSE4_1) != 0 ? "Yes" : "No");
    }
    if (__get_cpuid_max(0, NULL) >= 7) {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        printConstraint("CPU/SIMD", "AVX2 (256-bit) Support", (ebx & bit_AVX2) != 0 ? "Yes" : "No");
        printConstraint("CPU/SIMD", "AVX-512 Support", (ebx & bit_AVX512F) != 0 ? "Yes" : "No");
    }
}

void checkDataConstraints() {
    cout << "\n=== DATA TYPE CONSTRAINTS ===\n";
    printConstraint("Data Types", "Max Integer", to_string(numeric_limits<int>::max()));
    printConstraint("Data Types", "Max Unsigned 64-bit", to_string(numeric_limits<unsigned long long>::max()));
    printConstraint("Data Types", "Pointer Size", to_string(sizeof(void*)) + " Bytes");
}

int main() {
    cout << "=================================================================\n";
    cout << "               OMNI-CONSTRAINT ANALYZER V1.0                     \n";
    cout << "=================================================================\n";
    checkCompilerConstraints();
    checkOSConstraints();
    checkHardwareMemoryConstraints();
    checkCPUSIMDConstraints();
    checkDataConstraints();
    cout << "\n=================================================================\n";
    return 0;
}
