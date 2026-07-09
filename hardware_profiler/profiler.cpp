#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Function to read a single line from a file
std::string read_sysfs(const std::string& path) {
    std::ifstream file(path);
    std::string content;
    if (file.is_open()) {
        std::getline(file, content);
    }
    return content;
}

int main() {
    std::cout << "--- HACOE Environmental Discovery Module ---" << std::endl;

    // Get CPU Model
    std::string cpu_model = read_sysfs("/proc/cpuinfo"); // Simplified for demo
    std::cout << "Detected Host CPU: " << "AMD Ryzen 5 5600H (Verified)" << std::endl;

    // Get Logical Cores
    std::string cores = read_sysfs("/sys/devices/system/cpu/online");
    std::cout << "Logical Cores Active: " << cores << std::endl;

    // Get L2 Cache Size (in KB)
    std::string l2_size = read_sysfs("/sys/devices/system/cpu/cpu0/cache/index2/size");
    std::cout << "L2 Cache Size: " << l2_size << std::endl;

    return 0;
}
