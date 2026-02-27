#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <elf.h>
#include <fstream>
#include <libcpu/memory.hh>
#include <libvio/width.hh>
#include <memory>

namespace libcpu {

memory_view::memory_view(const memory_view &src, uint64_t src_base,
                         uint64_t view_base, uint64_t view_size) {
  mem_ptr = src.mem_ptr + intptr_t(view_base - src_base);
  base = view_base;
  size = view_size;
}

memory_view::memory_view() {}

memory::memory(uint64_t mem_base, size_t mem_size) {
  mem = std::unique_ptr<uint8_t[]>{new uint8_t[mem_size]};
  mem_ptr = mem.get();
  base = mem_base;
  size = mem_size;
}

std::optional<uint64_t> memory_view::read(uint64_t addr, libvio::width_t width) {
  if (out_of_bound(addr, width)) {
    return {};
  }

  const size_t start_offset = addr - base;
  const size_t w = static_cast<size_t>(width);

  uint64_t value = 0;
  for (size_t i = 0; i < w; i++) {
    value |= static_cast<uint64_t>(mem_ptr[start_offset + i]) << (i * 8);
  }
  return value;
}

bool memory_view::write(uint64_t addr, libvio::width_t width, uint64_t value) {
  const size_t start_offset = addr - base;
  const size_t w = static_cast<size_t>(width);

  if (out_of_bound(addr, width)) {
    return false;
  }

  for (size_t i = 0; i < w; i++) {
    mem_ptr[start_offset + i] = (value >> (i * 8)) & 0xFF;
  }
  return true;
}

uint8_t *memory_view::host_addr(uint64_t addr) {
  if (out_of_bound(addr, libvio::width_t::byte)) {
    return nullptr;
  }
  return mem_ptr + (addr - base);
}

void memory_view::save(const char *filename) const {
  std::ofstream out(filename, std::ios::binary);
  if (!out)
    return;
  save(out);
}

void memory_view::save(std::ostream &out) const {
  out.write(reinterpret_cast<const char *>(mem_ptr), size);
}

uint64_t memory_view::restore(const char *filename) {
  std::ifstream in(filename, std::ios::binary);
  if (!in)
    return 0;
  return restore(in);
}

uint64_t memory_view::restore(std::istream &in) {
  in.seekg(0, std::ios::end);
  const size_t file_size = in.tellg();
  in.seekg(0);
  const size_t bytes_to_read = std::min(file_size, static_cast<size_t>(size));
  in.read(reinterpret_cast<char *>(mem_ptr), bytes_to_read);
  return bytes_to_read;
}

template <typename WORD_T, typename EHDR_T, typename PHDR_T>
static inline WORD_T load_elf_impl(uint8_t *dest, const uint8_t *src,
                                   size_t offset) {
  EHDR_T *elf_header = (EHDR_T *)(src);
  // load metadata
  WORD_T entry = elf_header->e_entry;
  // load each segment
  PHDR_T *segment_headers = (PHDR_T *)(src + elf_header->e_phoff);
  for (size_t i = 0; i < elf_header->e_phnum; ++i) {
    if (segment_headers[i].p_type != PT_LOAD) {
      continue;
    }
    WORD_T seg_base = segment_headers[i].p_offset;
    WORD_T seg_size = segment_headers[i].p_memsz;
    WORD_T file_size = segment_headers[i].p_filesz;
    // if one of p_paddr and p_vaddr is zero, use the non-zero one
    // if both are non-zero but different, the behavior is undefined
    uint8_t *target_addr =
        dest + (segment_headers[i].p_vaddr | segment_headers[i].p_paddr) -
        offset;
    // load the content
    const uint8_t *seg_content = src + seg_base;
    std::copy(seg_content, seg_content + file_size, target_addr);
    // fill the remaining part with zero
    if (seg_size > file_size) {
      std::fill_n(target_addr + file_size, seg_size - file_size, 0);
    }
  }
  return entry;
}

uint64_t memory_view::load_elf(const uint8_t *buffer) {
  if (buffer[4] == ELFCLASS32) {
    return load_elf_impl<uint32_t, Elf32_Ehdr, Elf32_Phdr>(mem_ptr, buffer,
                                                           base);
  } else if (buffer[4] == ELFCLASS64) {
    return load_elf_impl<uint64_t, Elf64_Ehdr, Elf64_Phdr>(mem_ptr, buffer,
                                                           base);
  }
  return 0;
}

uint64_t memory_view::load_elf_from_file(const char *filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  const auto file_size = file.tellg();
  file.seekg(0);
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[file_size]);
  file.read(reinterpret_cast<char *>(buffer.get()), file_size);
  return load_elf(buffer.get());
}

uint64_t memory_view::get_size() const { return size; }

bool memory_view::out_of_bound(uint64_t addr, libvio::width_t width) const {
  const size_t up_addr = addr + static_cast<size_t>(width);
  return addr < base || up_addr > base + size;
}

} // namespace libcpu
