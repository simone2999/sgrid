#ifndef SGRID_IO_HPP
#define SGRID_IO_HPP

#include <filesystem>
#include <fstream>
#include "sgrid_Base.hpp"
#include "sgrid_Grid.hpp"

namespace sgrid {

    template <class Field>
    class IO {
    public:
        IO(){};
        IO(Field& x, const std::string& file_name) : field_(x){
            file_name_ = file_name;
            init();
        };


        void init(){
            auto g = field_.grid();
            auto g_host = g->view_host();
            if (Field::Grid::Dim != 3){
                meta = MetadataIO(g_host.dim[0],g_host.dim[1],g_host.dim[2], field_.block_size(), "Little");
            } else {
                meta = MetadataIO(g_host.dim[0],g_host.dim[1],0, field_.block_size(), "Little");
            }
        };


        void write(){
            meta.write();
        };

        class MetadataIO {
        public:
            MetadataIO(const int nx, const int ny, const int nz, const int block_size, const std::string& endianess) {
                nx_ = nx;
                ny_ = ny;
                nz_ = nz;
                block_size_ = block_size;
                endianess_ = endianess;
            };
            void write() {
                std::ofstream file(folder_path_.string() + "/" + "metadata.yml");
                std::ostringstream oss;
                oss << "nx: " << nx_ << "\nny: " << ny_ << "\nnz: " << nz_ << "\nendianess: " << endianess_
                    << "\nblock_size: " << block_size_ << std::endl;
                std::string text = oss.str();
                file << text;
            };
            int nx_, ny_, nz_, block_size_;
            std::string endianess_;
            std::filesystem::path folder_path_;
        };

    private:
        Field& field_;
        MetadataIO meta;
        std::string file_name_;
    };
}  // namespace sgrid

#endif