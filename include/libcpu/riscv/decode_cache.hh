#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libcpu/riscv/riscv.hh>
#include <libcpu/riscv/user_core.hh>
#include <vector>

namespace libcpu::riscv {

template <typename WORD_T, size_t offset_bits, size_t shamt>
class decode_cache {
public:
  static constexpr size_t capacity = ~(~size_t(0) << offset_bits) + 1;
  static constexpr WORD_T mask = ~(~WORD_T(0) << (offset_bits + shamt));

  std::vector<std::pair<uint32_t, decode_t>> cache{capacity};

  decode_cache(void);
  void decode(exec_result_t<WORD_T> &op);
};

template <typename WORD_T, size_t offset_bits, size_t shamt>
decode_cache<WORD_T, offset_bits, shamt>::decode_cache(void) {
  for (auto &entry : cache) {
    entry = {0,
             {.imm = 0,
              .dispatch = dispatch_t::invalid,
              .rs1 = 0,
              .rs2 = 0,
              .rd = 0}};
  }
}

template <typename WORD_T, size_t offset_bits, size_t shamt>
void decode_cache<WORD_T, offset_bits, shamt>::decode(
    exec_result_t<WORD_T> &op) {
  assert(op.type == exec_result_type_t::fetch);

  uint32_t offset = (op.pc & mask) >> shamt;
  auto cached = cache[offset];

  if (op.instr == cached.first) {
    op.type = exec_result_type_t::decode;
    op.decode = cached.second;
  } else {
    user_core<WORD_T>::decode(op);
    cache[offset] = {op.instr, op.decode};
  }

  assert(op.type == exec_result_type_t::decode);
}

} // namespace libcpu::riscv