#include <CommonCrypto/CommonDigest.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <filesystem>

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

int main(int argc, char* argv[]){
    if(argc != 2){
        std::cerr << "Usage: ./fim <directory>\n.";
        return 1;
    }

    fs::path directory = argv[1];

    try{
        if(!fs::is_directory(directory)){
            std::cerr << "Error: path is not a directory.\n";
            return 1;
        }
        size_t file_count = 0;

        for(const auto& entry:
        fs::recursive_directory_iterator(directory)){
            if(entry.is_symlink()){
                continue;
            }

            if(entry.is_regular_file()){
                const auto size = entry.file_size();
                const std::string hash = hash_file(entry.path());

                std::cout << entry.path().string() << " | " << size << "bytes" 
                << " | SHA-256: " << hash << '\n';
                ++file_count;
            }
        }
        std::cout << "Scan complete. Files found: "
                  << file_count << '\n';
    }catch(const std::exception& error){
        std::cerr << "Scan failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}