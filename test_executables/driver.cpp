#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string_view>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <cinttypes>
/**
 * @brief Read in a string of hex bytes and return a vector of bytes.
 *
 * @param[in] input String containing hex bytes.
 * @return Vector of bytes.
 */
static inline std::vector<uint8_t>
base16_decode(const std::string& input)
{
    std::vector<uint8_t> output;
    std::stringstream ss(input);
    std::string value;
    while (std::getline(ss, value, ' ')) {
        try {
            output.push_back(std::stoi(value, nullptr, 16));
        } catch (...) {
            // Ignore invalid values.
        }
    }
    return output;
}

void
write_file(const std::vector<uint8_t>& bytes, const std::string& file_name)
{
    std::ofstream os(file_name, std::ios::binary);
    assert(os.is_open());
    os.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    os.close();
}

int
main(int argc, char** argv)
{
    bool debug = false;
    bool elf = false;
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() > 0) {
        args.erase(args.begin());
    }
    std::string program_string;
    std::string memory_string;

    if (args.size() > 0 && args[0] == "--help") {
        std::cout << "usage: " << argv[0]
                  << " [--program <base16 program bytes>] [<base16 memory bytes>] [--debug] [--elf]" << std::endl;
        return 1;
    }

    if (args.size() > 1 && args[0] == "--program") {
        args.erase(args.begin());
        program_string = args[0];
        args.erase(args.begin());
    } else {
        std::getline(std::cin, program_string);
    }

    // Next parameter is optional memory contents.
    if (args.size() > 0 && !args[0].starts_with("--")) {
        memory_string = args[0];
        args.erase(args.begin());
    }

    if (args.size() > 0 && args[0] == "--debug") {
        debug = true;
        args.erase(args.begin());
    }

    if (args.size() > 0 && args[0] == "--elf") {
        elf = true;
        args.erase(args.begin());
    }

    if (args.size() > 0 && args[0].size() > 0) {
        std::cerr << "Unexpected arguments: " << args[0] << std::endl;
        return 1;
    }
    const char* runtime_excutable = getenv("RUNTIME_EXECUTABLE");
    if (runtime_excutable == nullptr) {
        std::cerr << "Please set RUNTIME_EXECUTABLE to a bpf-benchmark compatible runtime executable" << std::endl;
        return 1;
    }
    std::vector<uint8_t> memory = base16_decode(memory_string);
    std::vector<uint8_t> program_bytes = base16_decode(program_string);
    char tmp_dir_name[] = "/tmp/bpf_conformance.XXXXXX";

    mkdtemp(tmp_dir_name);
    // Write memory and program into files, so that the runtime executable could load them

    auto memory_file = std::string(tmp_dir_name) + "/memory";
    auto program_file = std::string(tmp_dir_name) + "/program";
    write_file(memory, memory_file);
    write_file(program_bytes, program_file);
    auto command = std::string(runtime_excutable) + " " + program_file + " " + memory_file;

    auto pipe = popen(command.c_str(), "r");
    uint64_t ret;
    int cnt = fscanf(pipe, "%*d %*d %" SCNu64, &ret);
    assert(cnt == 1);
    std::cout << std::hex << ret << std::endl;
    return 0;
}
