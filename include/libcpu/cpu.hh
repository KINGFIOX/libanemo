#ifndef LIBCPU_CPU_HH
#define LIBCPU_CPU_HH

#include <cstdint>
#include <cstddef>

class cpu {
    public:
        static constexpr size_t n_gpr = 0;
        static const char* gpr_num2name(uint8_t gpr_num);
        static const uint8_t gpr_name2num(uint8_t gpr_name);
};

#endif
