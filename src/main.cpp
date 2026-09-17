#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

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
        std::size_t file_count = 0;

        for(const auto& entry:
        fs::recursive_directory_iterator(directory)){
            if(entry.is_symlink()){
                continue;
            }

            if(entry.is_regular_file()){
                std::cout << entry.path().string() << " | " << entry.file_size() << "bytes\n";
                ++file_count;
            }
        }
        std::cout << "Scan complete. Files found: "
                  << file_count << '\n';
    }catch(const fs::filesystem_error& error){
        std::cerr << "Scan failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}