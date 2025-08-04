#include <cstddef>
#include <cstdint>
#include <libcpu/memory.hh>
#include <algorithm>
#include <elf.h>
#include <fstream>
#include <libvio/width.hh>

namespace libcpu {

memory::memory(uint64_t mem_base, size_t mem_size)
    : base(mem_base), size(mem_size), mem(new uint8_t[mem_size]{}) {}

std::optional<uint64_t> memory::read(uint64_t addr, libvio::width_t width, bool little_endian) {
    if (out_of_bound(addr, width)) {
        return {};
    }

    const size_t start_offset = addr - base;
    const size_t w = static_cast<size_t>(width);

    uint64_t value = 0;
    if (little_endian) {
        for (size_t i = 0; i < w; i++) {
            value |= static_cast<uint64_t>(mem[start_offset + i]) << (i * 8);
        }
    } else {
        for (size_t i = 0; i < w; i++) {
            value = (value << 8) | mem[start_offset + i];
        }
    }
    return value;
}

bool memory::write(uint64_t addr, libvio::width_t width, uint64_t value, bool little_endian) {
    const size_t start_offset = addr - base;
    const size_t w = static_cast<size_t>(width);
    
    if (out_of_bound(addr, width)) {
        return false;
    }

    if (little_endian) {
        for (size_t i = 0; i < w; i++) {
            mem[start_offset + i] = (value >> (i * 8)) & 0xFF;
        }
    } else {
        for (size_t i = 0; i < w; i++) {
            mem[start_offset + i] = (value >> ((w - 1 - i) * 8)) & 0xFF;
        }
    }
    return true;
}

uint8_t* memory::host_addr(uint64_t addr) {
    if (out_of_bound(addr, libvio::width_t::byte)) {
        return nullptr;
    }
    return mem.get() + (addr - base);
}

void memory::save(const char* filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out) return;
    save(out);
}

void memory::save(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(mem.get()), size);
}

uint64_t memory::restore(const char* filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) return 0;
    return restore(in);
}

uint64_t memory::restore(std::istream& in) {
    in.seekg(0, std::ios::end);
    const size_t file_size = in.tellg();
    in.seekg(0);
    const size_t bytes_to_read = std::min(file_size, static_cast<size_t>(size));
    in.read(reinterpret_cast<char*>(mem.get()), bytes_to_read);
    return bytes_to_read;
}

template <typename WORD_T, typename EHDR_T, typename PHDR_T>
static inline WORD_T load_elf_impl(uint8_t *dest, const uint8_t *src, size_t offset) {
    EHDR_T *elf_header = (EHDR_T*)(src);
    // load metadata
    WORD_T entry = elf_header->e_entry;
    // load each segment
    PHDR_T *segment_headers = (PHDR_T*)(src+elf_header->e_phoff);
    for (size_t i=0; i<elf_header->e_phnum; ++i) {
        if (segment_headers[i].p_type != PT_LOAD) {
            continue;
        }
        WORD_T seg_base = segment_headers[i].p_offset;
        WORD_T seg_size = segment_headers[i].p_memsz;
        WORD_T file_size = segment_headers[i].p_filesz;
        // if one of p_paddr and p_vaddr is zero, use the non-zero one
        // if both are non-zero but different, the behavior is undefined
        uint8_t *target_addr = dest + (segment_headers[i].p_vaddr | segment_headers[i].p_paddr) - offset;
        // load the content
        const uint8_t *seg_content = src + seg_base;
        std::copy(seg_content, seg_content+file_size, target_addr);
        // fill the remaining part with zero
        if (seg_size > file_size) {
            std::fill_n(target_addr+file_size, seg_size-file_size, 0);
        }
    }
    return entry;   
}

uint64_t memory::load_elf(const uint8_t* buffer) {
    if (buffer[4] == ELFCLASS32) {
        return load_elf_impl<uint32_t, Elf32_Ehdr, Elf32_Phdr>(mem.get(), buffer, base);
    } else if (buffer[4] == ELFCLASS64) {
        return load_elf_impl<uint64_t, Elf64_Ehdr, Elf64_Phdr>(mem.get(), buffer, base);
    }
    return 0;
}

uint64_t memory::load_elf_from_file(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    const auto file_size = file.tellg();
    file.seekg(0);
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[file_size]);
    file.read(reinterpret_cast<char*>(buffer.get()), file_size);
    return load_elf(buffer.get());
}

uint64_t memory::get_size() const {
    return size;
}

bool memory::out_of_bound(uint64_t addr, libvio::width_t width) const {
    const size_t up_addr = addr + static_cast<size_t>(width);
    return addr < base || up_addr > base + size;
}

} // namespace libcpu
