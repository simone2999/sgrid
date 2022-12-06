#include "sgrid_Utils.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include "sgrid_Base.hpp"

#include <mpi.h>

namespace sgrid {
    void ensure_path_exists(const std::string &path) {
        int pos = path.find('/');
        // Check if given path is just a file or specifies a folder.
        if (pos > 0) {
            std::string folder = path.substr(0, pos);
            std::filesystem::path p = folder;
            std::filesystem::path absolute_path = std::filesystem::absolute(p);
            if (!std::filesystem::exists(absolute_path)) {
                std::filesystem::create_directory(folder);
            } else if (std::filesystem::exists(absolute_path) && !std::filesystem::is_directory(absolute_path)) {
                std::cerr << "[Error] Folder already exists as a file.";
                assert(false);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        } else {
            std::filesystem::path p = path;
            std::filesystem::path absolute_path = std::filesystem::absolute(p);
            if (std::filesystem::exists(absolute_path) && std::filesystem::is_directory(absolute_path)) {
                std::cerr << "[Error] Cannot write file. It already exists as a directory." << std::endl;
                assert(false);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
    }
}  // namespace sgrid