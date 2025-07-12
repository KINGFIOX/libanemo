#ifndef LIBCPU_MEMORY_HH
#define LIBCPU_MEMORY_HH

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <elf.h>
#include <fstream>
#include <libvio/frontend.hh>
#include <memory>
#include <optional>

namespace libcpu {

/**
 * @brief Abstract base class template for memory interfaces.
 * 
 * This class defines the interface for memory operations that can be implemented
 * by concrete memory classes. It provides pure virtual functions for reading,
 * writing, and accessing memory. This class only provides architecture independent
 * physical memory interface. It is the architecture that defines the behavior of
 * virtual memory.
 * 
 * @tparam WORD_T The type for address and data. Must be an integral type.
 */
template <typename WORD_T>
class abstract_memory {
public:
    /**
     * @brief Read from memory at the specified address. This might have side-effects like caching.
     * 
     * @param addr The memory address to read from.
     * @param width The width of the data to read.
     * @param little_endian If true, use little-endian byte ordering (default).
     *                      If false, use big-endian byte ordering.
     * @return std::optional<WORD_T> The zero extended read value if successful, or std::nullopt if failed.
     *
     * @note This function is designed for similating the memory access mechanism.
     *      To read the memory content for debugging, use `peek()` or `host_addr()`.
     */
    virtual std::optional<WORD_T> read(WORD_T addr, libvio::width_t width, bool little_endian=true) = 0;

    /**
     * @brief Read from memory at the specified address. This has no side-effects like caching.
     * 
     * @param addr The memory address to read from.
     * @param width The width of the data to read.
     * @param little_endian If true, use little-endian byte ordering (default).
     *                      If false, use big-endian byte ordering.
     * @return std::optional<WORD_T> The zero extended read value if successful, or std::nullopt if failed.
     *
     * @note This function is designed to access the memory content for debugging.
     */
    virtual std::optional<WORD_T> peek(WORD_T addr, libvio::width_t width, bool little_endian=true) const = 0;

    /**
     * @brief Write to memory at the specified address. This might have side-effects like caching.
     * 
     * @param addr The memory address to write to.
     * @param width The width of the data to write.
     * @param little_endian If true, use little-endian byte ordering (default).
     *                      If false, use big-endian byte ordering.
     * @return bool True if the write was successful, false otherwise.
     *
     * @note This function is designed for similating the memory access mechanism.
     *      To write the memory content for debugging or initailizing, use `set()` or `host_addr()`.
     */
    virtual bool write(WORD_T addr, libvio::width_t width, WORD_T value, bool little_endian=true) = 0;

    /**
     * @brief Write to memory at the specified address. This has no side-effects like caching.
     * 
     * @param addr The memory address to write to.
     * @param width The width of the data to write.
     * @param little_endian If true, use little-endian byte ordering (default).
     *                      If false, use big-endian byte ordering.
     * @return bool True if the write was successful, false otherwise.
     *
     * @note This function is designed for debugging or initailizing.
     */
    virtual bool set(WORD_T addr, libvio::width_t width, WORD_T value, bool little_endian=true) = 0;

    /**
     * @brief Get a pointer to the host memory at the specified address.
     * 
     * @param addr The memory address to access.
     * @param value The value to write.
     * @return uint8_t* Pointer to the host memory at the specified address,
     *                  or nullptr if the address is invalid or this operation is not supported.
     *
     * @note This is intended to provide a convenient and effective way for the debugger to access the memory content.
     *      It has no magic to trigger side-effects like caching.
     */
    virtual uint8_t *host_addr(WORD_T addr) = 0;

    /**
     * @brief Save the memory contents to a file.
     * 
     * @param filename The name of the file to save to.
     */
    virtual void save(const char *filename) const = 0;

    /**
     * @brief Restore the memory contents from a file.
     * 
     * @param filename The name of the file to restore from.
     * @returns The actual size loaded.
     *
     * @note Different subclasses can use different formats for its checkpoint files.
     */
    virtual WORD_T restore(const char *filename) = 0;

    using elf_hdr_t = std::conditional_t<sizeof(WORD_T) == 4, Elf32_Ehdr, Elf64_Ehdr>;
    using elf_phdr_t = std::conditional_t<sizeof(WORD_T) == 4, Elf32_Phdr, Elf64_Phdr>;

    /**
     * @brief Load an ELF binary from memory into the emulated memory space.
     * 
     * This method parses the ELF header and program headers, then loads all loadable
     * segments (PT_LOAD) into the emulated memory. The segments are copied from the
     * buffer to their specified virtual addresses.
     * 
     * @param buffer Pointer to the ELF binary data in memory
     * @return WORD_T The entry point address specified in the ELF header
     * 
     * @note The ELF binary must match the architecture's word size (32-bit or 64-bit)
     * @note Only PT_LOAD segments are processed, other segment types are ignored
     */
    virtual WORD_T load_elf(const uint8_t *buffer) {
        elf_hdr_t *elf_header = (elf_hdr_t*)(buffer);
        // load metadata
        WORD_T entry = elf_header->e_entry;
        // load each segment
        elf_phdr_t *segment_headers = (elf_phdr_t*)(buffer+elf_header->e_phoff);
        for (size_t i=0; i<elf_header->e_phnum; ++i) {
            if (segment_headers[i].p_type != PT_LOAD) {
                continue;
            }
            WORD_T seg_base = segment_headers[i].p_offset;
            WORD_T seg_size = segment_headers[i].p_memsz;
            WORD_T file_size = segment_headers[i].p_filesz;
            // if one of p_paddr and p_vaddr is zero, use the non-zero one
            // if both are non-zero but different, the behavior is undefined
            uint8_t *target_addr = host_addr(segment_headers[i].p_vaddr | segment_headers[i].p_paddr);
            if (target_addr == nullptr) {
                continue;
            }
            // load the content
            const uint8_t *seg_content = buffer + segment_headers[i].p_offset;
            std::copy(seg_content, seg_content+file_size, target_addr);
            // fill the remaining part with zero
            if (seg_size > file_size) {
                std::fill_n(target_addr+file_size, seg_size-file_size, 0);
            }
        }
        return entry;   
    }

    /**
     * @brief Load an ELF binary from a file into the emulated memory space.
     * 
     * This method reads an ELF file from disk and loads it using the same logic
     * as load_elf(). The file is read into memory and then processed.
     * 
     * @param filename Path to the ELF file to load
     * @return WORD_T The entry point address specified in the ELF header
     */
    virtual WORD_T load_elf_from_file(const char *filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        auto filesize = file.tellg();
        std::unique_ptr<uint8_t[]> buffer{new uint8_t[filesize]};
        file.seekg(0);
        file.read((char*)(buffer.get()), filesize);
        return load_elf(buffer.get());
    }
};

/**
 * @class contiguous_memory
 * @brief Concrete memory implementation using contiguous byte-array storage.
 */
template <typename WORD_T>
class contiguous_memory: public abstract_memory<WORD_T> {
    protected:
        WORD_T base;
        WORD_T size;
        std::unique_ptr<uint8_t[]> mem;
    public:
        /**
        * @brief Construct a new vector memory object
        * 
        * @param mem_base Base address of the memory region
        * @param mem_size Size of memory region in bytes
        */
        contiguous_memory(WORD_T mem_base, size_t mem_size) {
            base = mem_base;
            size = mem_size;
            mem = std::unique_ptr<uint8_t[]>{new uint8_t[mem_size]};
        }

        bool out_of_bound(WORD_T addr, libvio::width_t width) const {
            size_t up_addr = addr + static_cast<size_t>(width);
            return addr < base || up_addr > base+size;
        }

        WORD_T get_size(void) {
            return size;
        }

        std::optional<WORD_T> read(WORD_T addr, libvio::width_t width, bool little_endian=true) override {
            return peek(addr, width, little_endian);
        }
    
        std::optional<WORD_T> peek(WORD_T addr, libvio::width_t width, bool little_endian=true) const override {
            if (out_of_bound(addr, width)) {
                return {};
            }

            size_t start_offset = addr - base;
            size_t w = static_cast<size_t>(width);

            WORD_T value = 0;
            if (little_endian) {
                for (size_t i = 0; i<w; i++) {
                    value |= static_cast<WORD_T>(mem[start_offset+i]) << (i*8);
                }
            } else {
                for (size_t i = 0; i<w; i++) {
                    value = (value << 8) | mem[start_offset+i];
                }
            }
            return value;
        }

        bool write(WORD_T addr, libvio::width_t width, WORD_T value, bool little_endian=true) override {
            return set(addr, width, value, little_endian);
        }

        bool set(WORD_T addr, libvio::width_t width, WORD_T value, bool little_endian=true) override {
            size_t start_offset = addr - base;
            size_t w = static_cast<size_t>(width);
            
            if (out_of_bound(addr, width)) {
                return false;
            }

            if (little_endian) {
                for (size_t i = 0; i<w; i++) {
                    mem[start_offset+i] = (value >> (i*8)) & 0xFF;
                }
            } else {
                for (size_t i = 0; i<w; i++) {
                    mem[start_offset+i] = (value >> ((w-1-i)*8)) & 0xFF;
                }
            }
            return true;
        }

        uint8_t* host_addr(WORD_T addr) override {
            if (out_of_bound(addr, libvio::width_t::byte)) {
                return nullptr;
            } else {
                return mem.get() + (addr-base);
            }
        }

        void save(const char* filename) const override {
            std::ofstream out(filename, std::ios::binary);
            if (!out) return;
            out.write(reinterpret_cast<const char*>(mem.get()), size);
        }

        WORD_T restore(const char* filename) override {
            std::ifstream in(filename, std::ios::binary | std::ios::ate);
            if (!in) return 0;
            
            size_t file_size = in.tellg();
            in.seekg(0);
            size_t bytes_to_read = std::min(file_size, size_t(size));
            
            in.read(reinterpret_cast<char*>(mem.get()), bytes_to_read);
            return bytes_to_read;
        }
};

}

#endif
