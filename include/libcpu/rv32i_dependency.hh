#ifndef LIBCPU_DEPENDENCY_HH
#define LIBCPU_DEPENDENCY_HH

#include <cstdint>
#include <optional>
#include <tuple>
#include <map>
#include <libcpu/rv32i.hh>

namespace libcpu::rv32i {

// The values that an instruction reads from
struct static_source {
    uint8_t rs1;
    uint8_t rs2;
    bool csr;
    std::optional<uint64_t> mem_addr;
};

// the values that an instruction in an inctruction stream reads from
// each time a source is written, xxx_no is increased by 1
// so this structure becomes an SSA form
struct dynamic_source {
    uint8_t rs1;
    uint8_t rs2;
    bool csr;
    std::optional<uint64_t> mem_addr;
    uint64_t rs1_no;
    uint64_t rs2_no;
    uint64_t mem_addr_no;
};

// The values that an instruction writes to
struct static_sink {
    uint8_t rd;
    bool csr;
    std::optional<uint64_t> mem_addr;
};

// the values that an instruction in an inctruction stream writes to
// similar with dynamic_source
struct dynamic_sink {
    uint8_t rd;
    bool csr;
    std::optional<uint64_t> mem_addr;
    uint64_t rd_no;
    uint64_t mem_addr_no;
};

std::tuple<static_source, static_sink> decode_static_dependency(uint32_t instruction);

class dynamic_dependency_analyzer {
    private:
        uint64_t reg_write_count[32] = {0, };
        std::map<uint64_t, uint64_t> mem_write_count;
        dynamic_source src_scratch = {0, 0, false, {}, 0, 0, 0};
        dynamic_sink sink_scratch = {0, false, {}, 0, 0};
    public:
        /* std::tuple<dynamic_source, dynamic_sink> next_instruction(std::tuple<static_source, static_sink> static_dependency);
        std::tuple<dynamic_source, dynamic_sink> next_instruction(uint32_t instruction); */
        std::optional<std::tuple<dynamic_source, dynamic_sink>> next_event(event_t event);
};

}

#endif