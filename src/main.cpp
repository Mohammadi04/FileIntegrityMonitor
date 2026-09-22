#include <CommonCrypto/CommonDigest.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

//Hashing functoin 
/**
 * Hasing function will Open the file in binary mode to read its exact bytes.
 * Initialize a SHA-256 calculation.
 * Read up to 8 KB at a time, keeping memory usage small.
 * Feed each chunk into the calculation. 
 * gcount() tells us how many bytes were actually read.
 * Check whether reading ended normally.
 * Finish the hash and convert its 32 bytes 
 * into 64 hexadecimal characters.
 */
std::string hash_file(const fs::path& path){
    std::ifstream file(path, std::ios::binary);

    if(!file.is_open()){
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    CC_SHA256_CTX context;
    CC_SHA256_Init(&context);

    char buffer[8192];

    while(file.read(buffer, sizeof(buffer)) || file.gcount() > 0){
        CC_SHA256_Update(&context, buffer, static_cast<CC_LONG>(file.gcount()));
    }

    if(file.bad() || !file.eof()){
        throw std::runtime_error("Cannot read file: " + path.string());
    }

    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(digest, & context);

    std::ostringstream result;
    result << std::hex << std::setfill('0');

    for(unsigned char byte: digest){
        result << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return result.str();
}

/**
 * It returns a map containing relative file paths → hashes.
 * Relative paths such as nested/another.txt identify files within the monitored directory.
 * It skips symbolic links, as before.
 * If scanning or hashing fails, it throws an exception instead of returning incomplete results.
 */
std::map<std::string, std::string> scan_directory(const fs::path& directory){
    if(!fs::is_directory(directory)){
        throw std::runtime_error("Path is not a directory");
    }

    std::map<std::string, std::string> snapshot;

    for(const auto& entry : fs::recursive_directory_iterator(directory)){
        if(entry.is_symlink()){
            continue;
        }

        if(entry.is_regular_file()){
            const std::string relative_path = entry.path().lexically_relative(directory).generic_string();

            const auto size = entry.file_size();
            const std::string hash = hash_file(entry.path());

            snapshot.emplace(relative_path, hash);

            std::cerr << relative_path << " | " << size << " bytes" <<
            " | SHA-256: " << hash << '\n';
        }
    }
    return snapshot;
}
std::string json_string(const std::string& text){
    std::ostringstream output;
    output << '"';

    for(unsigned char ch: text){
        if(ch == '"'){
            output << "\\\"";
        }else if(ch == '\\'){
            output << "\\\\";
        }else if(ch < 0x20){
            output << "\\u" 
                   << std::hex << std::setw(4)
                   << std::setfill('0') << static_cast<unsigned int>(ch);
        }else{
            output << static_cast<char>(ch);
        }
    }
    output << '"';
    return output.str();
}
void print_change(const std::string& status, const std::string& path){
    std::cout << "{\"type\":\"change\",\"status\":"
              << json_string(status)
              << ",\"path\":"
              << json_string(path)
              << "}\n";
}
/**
 * Reads the saved directory and file hashes.
 * Scans that directory again using your existing function.
 * Compares the two maps.
 */
void check_baseline() {
    std::ifstream input("baseline.txt");

    if (!input.is_open()) {
        throw std::runtime_error("Cannot open baseline.txt.");
    }

    std::string header;
    std::getline(input, header);

    if (header != "FIM_BASELINE_V1") {
        throw std::runtime_error("Unsupported baseline format.");
    }

    std::string directory_text;

    input >> std::ws;

    if (input.peek() != '"' ||
        !(input >> std::quoted(directory_text))) {
        throw std::runtime_error("Invalid baseline directory.");
    }

    const fs::path directory(directory_text);

    if (!directory.is_absolute()) {
        throw std::runtime_error(
            "Baseline directory must be an absolute path."
        );
    }

    std::map<std::string, std::string> original;

    while (true) {
        input >> std::ws;

        if (input.bad()) {
            throw std::runtime_error("Failed to read baseline.");
        }

        if (input.eof()) {
            break;
        }

        std::string path;
        std::string hash;

        if (input.peek() != '"' ||
            !(input >> std::quoted(path) >> hash)) {
            throw std::runtime_error("Malformed baseline record.");
        }

        if (path.empty() ||
            hash.size() != 64 ||
            hash.find_first_not_of("0123456789abcdef")
                != std::string::npos) {
            throw std::runtime_error("Invalid baseline path or hash.");
        }

        if (!original.emplace(path, hash).second) {
            throw std::runtime_error(
                "Duplicate baseline path: " + path
            );
        }
    }

    const auto current = scan_directory(directory);
    std::size_t changes = 0;

    for (const auto& [path, hash] : current) {
        const auto previous = original.find(path);

        if (previous == original.end()) {
            print_change("ADDED", path);
            ++changes;
        } else if (previous->second != hash) {
            print_change("MODIFIED", path);
            ++changes;
        }
    }

    for (const auto& [path, hash] : original) {
        if (current.find(path) == current.end()) {
            print_change("DELETED", path);
            ++changes;
        }
    }
    /*
    std::cout << "Check complete. Changes found: "
              << changes << '\n';
    */
    std::cout << "{\"type\":\"summary\",\"changes\":"
        << changes << "}\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        //std::cerr << "Usage: ./fim <directory>\n";
        //return 1;

        std::cerr << "Usage: ./fim <directory> (create baseline)\n"
                  << "       ./fim --check.    (compare with baseline)\n";
    }

    try {

        if(std::string(argv[1]) == "--check"){
            check_baseline();
            return 0;
        }
        const fs::path directory = fs::canonical(argv[1]);
        const fs::path baseline =
            fs::current_path() / "baseline.txt";

        // Reject saving the baseline inside the monitored directory.
        const fs::path relative = baseline.lexically_relative(directory);

        if (!relative.empty() && *relative.begin() != "..") {
            throw std::runtime_error(
                "baseline.txt must be outside the monitored directory."
            );
        }

        if (fs::exists(baseline)) {
            throw std::runtime_error(
                "baseline.txt already exists; refusing to overwrite it."
            );
        }

        const auto snapshot = scan_directory(directory);

        std::ofstream output(baseline);

        if (!output.is_open()) {
            throw std::runtime_error("Cannot create baseline.txt.");
        }

        output << "FIM_BASELINE_V1\n";
        output << std::quoted(directory.generic_string()) << '\n';

        for (const auto& [path, hash] : snapshot) {
            output << std::quoted(path) << ' ' << hash << '\n';
        }

        output.close();

        if (!output) {
            throw std::runtime_error(
                "Failed to finish writing baseline.txt; "
                "the file may be incomplete."
            );
        }

        std::cout << "Baseline saved. Files recorded: "
                  << snapshot.size() << '\n';

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}