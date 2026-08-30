#include <cpuid.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

namespace {

std::optional<std::string> read_first_line(const std::string &path) {
    std::ifstream input(path);
    std::string value;
    if (!input || !std::getline(input, value)) {
        return std::nullopt;
    }
    return value;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::optional<std::string> cpuinfo_value(std::string_view key) {
    std::ifstream input("/proc/cpuinfo");
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }
        if (trim(line.substr(0, separator)) == key) {
            return trim(line.substr(separator + 1));
        }
    }
    return std::nullopt;
}

std::string json_escape(std::string_view value) {
    std::ostringstream output;
    for (const unsigned char character : value) {
        switch (character) {
        case '\\': output << "\\\\"; break;
        case '"': output << "\\\""; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            output << (character < 0x20 ? '?' : static_cast<char>(character));
        }
    }
    return output.str();
}

std::optional<std::uint64_t> read_xcr0() {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return std::nullopt;
    }
    constexpr unsigned int Osxsave = 1U << 27U;
    if ((ecx & bit_AVX) == 0 || (ecx & Osxsave) == 0) {
        return std::nullopt;
    }
    unsigned int xcr0_eax = 0;
    unsigned int xcr0_edx = 0;
    __asm__ volatile("xgetbv" : "=a"(xcr0_eax), "=d"(xcr0_edx) : "c"(0));
    return (static_cast<std::uint64_t>(xcr0_edx) << 32U) | xcr0_eax;
}

bool os_supports_avx() {
    const auto xcr0 = read_xcr0();
    return xcr0 && ((*xcr0 & 0x6U) == 0x6U);
}

bool os_supports_avx512() {
    const auto xcr0 = read_xcr0();
    constexpr std::uint64_t RequiredState = 0xE6U;
    return xcr0 && ((*xcr0 & RequiredState) == RequiredState);
}

struct SimdCapabilities {
    bool sse41 = false;
    bool avx = false;
    bool avx2 = false;
    bool avx512f = false;
};

SimdCapabilities detect_simd() {
    SimdCapabilities result;
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        result.sse41 = (ecx & bit_SSE4_1) != 0;
        result.avx = os_supports_avx();
    }
    if (__get_cpuid_max(0, nullptr) >= 7) {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        result.avx2 = result.avx && (ebx & bit_AVX2) != 0;
        result.avx512f = os_supports_avx512() && (ebx & bit_AVX512F) != 0;
    }
    return result;
}

std::string value_or_unknown(const std::optional<std::string> &value) {
    return value.value_or("unknown");
}

void print_json() {
    const auto simd = detect_simd();
    const auto model = value_or_unknown(cpuinfo_value("model name"));
    const auto vendor = value_or_unknown(cpuinfo_value("vendor_id"));
    const auto active_cpus = value_or_unknown(read_first_line("/sys/devices/system/cpu/online"));
    const auto l1d = value_or_unknown(read_first_line("/sys/devices/system/cpu/cpu0/cache/index0/size"));
    const auto l1i = value_or_unknown(read_first_line("/sys/devices/system/cpu/cpu0/cache/index1/size"));
    const auto l2 = value_or_unknown(read_first_line("/sys/devices/system/cpu/cpu0/cache/index2/size"));
    const auto l3 = value_or_unknown(read_first_line("/sys/devices/system/cpu/cpu0/cache/index3/size"));

    std::cout << "{\n"
              << "  \"schema_version\": 1,\n"
              << "  \"cpu\": {\n"
              << "    \"vendor\": \"" << json_escape(vendor) << "\",\n"
              << "    \"model\": \"" << json_escape(model) << "\",\n"
              << "    \"logical_cores\": " << std::thread::hardware_concurrency() << ",\n"
              << "    \"active_cpu_range\": \"" << json_escape(active_cpus) << "\"\n"
              << "  },\n"
              << "  \"cache\": {\n"
              << "    \"l1_data\": \"" << json_escape(l1d) << "\",\n"
              << "    \"l1_instruction\": \"" << json_escape(l1i) << "\",\n"
              << "    \"l2\": \"" << json_escape(l2) << "\",\n"
              << "    \"l3\": \"" << json_escape(l3) << "\"\n"
              << "  },\n"
              << "  \"simd\": {\n"
              << "    \"sse4_1\": " << std::boolalpha << simd.sse41 << ",\n"
              << "    \"avx_os_enabled\": " << simd.avx << ",\n"
              << "    \"avx2\": " << simd.avx2 << ",\n"
              << "    \"avx512f\": " << simd.avx512f << "\n"
              << "  }\n"
              << "}\n";
}

void print_human() {
    const auto simd = detect_simd();
    std::cout << "HACOE hardware profile\n"
              << "CPU model: " << value_or_unknown(cpuinfo_value("model name")) << '\n'
              << "CPU vendor: " << value_or_unknown(cpuinfo_value("vendor_id")) << '\n'
              << "Logical cores: " << std::thread::hardware_concurrency() << '\n'
              << "Active CPUs: "
              << value_or_unknown(read_first_line("/sys/devices/system/cpu/online")) << '\n'
              << "L2 cache: "
              << value_or_unknown(read_first_line("/sys/devices/system/cpu/cpu0/cache/index2/size")) << '\n'
              << "AVX OS-enabled: " << std::boolalpha << simd.avx << '\n'
              << "AVX2: " << simd.avx2 << '\n'
              << "AVX-512F: " << simd.avx512f << '\n';
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 1) {
        print_human();
        return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--json") {
        print_json();
        return 0;
    }
    std::cerr << "Usage: " << argv[0] << " [--json]\n";
    return 2;
}
