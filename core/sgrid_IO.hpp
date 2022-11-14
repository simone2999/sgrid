#ifndef SGRID_IO_HPP
#define SGRID_IO_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include "sgrid_Base.hpp"
#include "sgrid_Grid.hpp"

namespace sgrid {

    template <class Field>
    class IO {
    public:
        IO(){};
        IO(Field& x, const std::string& folder_name, bool time_series = false)
            : field_(x), folder_name_(folder_name), file_name_("data.raw"), time_series_(time_series) {
            init();
        }
        IO(Field& x, const std::string& folder_name, const std::string file_name, bool time_series = false)
            : field_(x), folder_name_(folder_name), file_name_(file_name), time_series_(time_series) {
            init();
        }
        /**
         * Modified constructor. Find out what the dim of the Field is so we know 2D or 3D.
         */
        void init() {
            auto g = field_.grid();
            auto g_host = g->view_host();
            if (Field::Grid::Dim == 2) {
                meta = MetadataIO(g_host.global_dim[0],
                                  g_host.global_dim[1],
                                  0,
                                  field_.block_size(),
                                  folder_name_,
                                  field_.get_value_type());
            } else if (Field::Grid::Dim == 3) {
                meta = MetadataIO(g_host.global_dim[0],
                                  g_host.global_dim[1],
                                  g_host.global_dim[2],
                                  field_.block_size(),
                                  folder_name_,
                                  field_.get_value_type());
            } else {
                assert(false);
            }
        }

        /**
         * Write everything:
         * Metadata: Static or time dependent.
         */
        void write() {
            auto grid = field_.grid();
            if (time_series_) {
                if (grid->comm_rank() == 0) {
                    meta.write(file_name_);
                }
                std::string new_raw_file_name = file_name_;
                size_t pos = new_raw_file_name.find(".raw");
                new_raw_file_name.insert(pos, "_t" + std::to_string(file_counter));
                field_.write(folder_name_ + '/' + new_raw_file_name);
                file_counter++;
            } else {
                if (grid->comm_rank() == 0) {
                    meta.write(file_name_);
                }
                field_.write(folder_name_ + '/' + file_name_);
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
                       std::string type,
                       std::string endianess = "Little") {
                nx_ = nx;
                ny_ = ny;
                nz_ = nz;
                block_size_ = block_size;
                folder_path_ = std::move(folder_path);
                endianess_ = std::move(endianess);
                type_ = std::move(type);
            }
            /**
             * Write the metadata.yml file containing: nx, ny, nz, block_size, endianess.
             */
            void write(std::string raw_file_name) {
                if (image_counter == 1) {
                    check_folder_exists();
                }

                size_t pos = raw_file_name.find(".raw");
                std::string file_name = raw_file_name.erase(pos, raw_file_name.length());
                std::ofstream file(folder_path_ + "/" + "metadata" + "_" + file_name + ".yml");
                std::ostringstream oss;
                oss << "nx: " << nx_ << "\nny: " << ny_ << "\nnz: " << nz_ << "\nendianess: " << endianess_
                    << "\nblock_size: " << block_size_ << "\ntype: " << type_ << "\ntime_steps: " << image_counter;
                std::string text = oss.str();
                file << text;
                image_counter++;
            }

            /**
             * Check if the folder_path_ given when creating a MetadataIO object already exists in the current
             * directory.
             */
            void check_folder_exists() {
                int pos = folder_path_.find('/');
                std::string folder = folder_path_.substr(0, pos);
                if (std::filesystem::exists(folder)) {
                    std::cout << "Folder exists" << std::endl;
                    std::cout << "Writing images:" << std::endl;
                } else {
                    std::filesystem::create_directory(folder);
                    std::cout << "Writing images:" << std::endl;
                }
            }
            int nx_{}, ny_{}, nz_{}, block_size_{};
            std::string endianess_;
            std::string folder_path_;
            std::string type_;
            int image_counter = 1;
        };

    private:
        Field& field_;
        MetadataIO meta;
        std::string folder_name_;
        std::string file_name_;
        int file_counter = 0;
        bool time_series_;
    };
}  // namespace sgrid

#endif