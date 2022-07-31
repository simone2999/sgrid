#ifndef SGRID_DATA_EXPORT_HPP
#define SGRID_DATA_EXPORT_HPP

#include <string.h>

struct data_export {
    int nx;
    int ny;
    int nz;
    int block_size;
    std::string endianess;
};

namespace sgrid {
    class DataExport {
    public:
        DataExport(){};
        DataExport(struct data_export data) {
            _nx = data.nx;
            _ny = data.ny;
            _nz = data.nz;
            _endianess = data.endianess;
            _block_size = data.block_size;
        };

        void create_header() {
            std::ofstream file("xdmf_data.txt");
            std::ostringstream oss;
            oss << "nx:" << _nx << "\nny:" << _ny << "\nnz:" << _nz << "\nendianess:" << _endianess
                << "\nblock_size:" << _block_size << std::endl;
            std::string text = oss.str();
            // std::cout << text;
            file << text;
        };

    private:
        int _nx;
        int _ny;
        int _nz;
        std::string _endianess;
        int _block_size;
    };
}  // namespace sgrid
#endif
