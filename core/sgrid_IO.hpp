#ifndef SGRID_IO_HPP
#define SGRID_IO_HPP

#include <filesystem>
#include <fstream>
#include <utility>
#include "sgrid_Base.hpp"
#include "sgrid_Grid.hpp"

namespace sgrid {

    template <class Field>
    class IO {
    public:
        IO(){};
        IO(Field& x, const std::string& folder_name) : field_(x) {
            folder_name_ = folder_name;
            init();
        };
        ~IO(){destroy();}
        /**
         * Modified constructor. Find out what the dim of the Field is so we know 2D or 3D.
         */
        void init() {
            auto g = field_.grid();
            auto g_host = g->view_host();
            if (Field::Grid::Dim != 3) {
                meta = MetadataIO(g_host.global_dim[0], g_host.global_dim[1], 0, field_.block_size(), folder_name_);
            } else {
                meta = MetadataIO(g_host.global_dim[0],
                                  g_host.global_dim[1],
                                  g_host.global_dim[2],
                                  field_.block_size(),
                                  folder_name_);
            }
        };

        /**
         * Write everything:
         * Metadata: Static or time dependent.
         * TODO: Time dependent.
         */
        void write(const std::string& raw_file_name) {
            auto grid = field_.grid();
            MPI_Comm comm = grid->raw_comm();
            int rank;
            MPI_Comm_rank(comm, &rank);

            if (rank == 0) {
                meta.write();
                field_.write(folder_name_ + '/' + raw_file_name);
            }
        }

        class MetadataIO {
        public:
            MetadataIO() = default;
            MetadataIO(const int nx,
                       const int ny,
                       const int nz,
                       const int block_size,
                       std::string folder_path,
                       std::string endianess = "Little") {
                nx_ = nx;
                ny_ = ny;
                nz_ = nz;
                block_size_ = block_size;
                folder_path_ = std::move(folder_path);
                endianess_ = std::move(endianess);
            };
            /**
             * Write the metadata.yml file containing: nx, ny, nz, block_size, endianess.
             */
            void write() {
                check_folder_exists();
                std::ofstream file(folder_path_ + "/" + "metadata.yml");
                std::ostringstream oss;
                oss << "nx: " << nx_ << "\nny: " << ny_ << "\nnz: " << nz_ << "\nendianess: " << endianess_
                    << "\nblock_size: " << block_size_ << std::endl;
                std::string text = oss.str();
                file << text;
            };

            /**
             * Check if the folder_path_ given when creating a MetadataIO object already exists in the current
             * directory.
             */
            void check_folder_exists() {
                int pos = folder_path_.find('/');
                std::string folder = folder_path_.substr(0, pos);
                if (std::filesystem::exists(folder)) {
                    std::cout << "Folder exists" << std::endl;
                } else {
                    std::filesystem::create_directory(folder);
                }
            }
            int nx_{}, ny_{}, nz_{}, block_size_{};
            std::string endianess_;
            std::string folder_path_;
        };

    private:
        Field& field_;
        MetadataIO meta;
        std::string folder_name_;

        MPI_Datatype interior_subarray_type_{MPI_DATATYPE_NULL};
        MPI_Datatype view_type_{MPI_DATATYPE_NULL};

        void destroy() {
            if (interior_subarray_type_ != MPI_DATATYPE_NULL) {
                MPI_Type_free(&interior_subarray_type_);
            }

            if (view_type_ != MPI_DATATYPE_NULL) {
                MPI_Type_free(&view_type_);
            }
        }
    };
}  // namespace sgrid

#endif