#ifndef SGRID_DATA_EXPORT_HPP
#define SGRID_DATA_EXPORT_HPP

#include <string.h>

namespace sgrid {
    class DataExport {
    public:
        DataExport(){};
        DataExport(const int nx, const int ny, const int nz, const int block_size, const std::string endianess) {
            _nx = nx;
            _ny = ny;
            _nz = nz;
            _endianess = endianess;
            _block_size = block_size;
        }

    private:
        int _nx;
        int _ny;
        int _nz;
        std::string _endianess;
        int _block_size;
    };
}  // namespace sgrid
#endif
