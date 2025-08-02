#ifndef LIBCPU_MEMORY_STAGING_HH
#define LIBCPU_MEMORY_STAGING_HH

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <libvio/width.hh>
#include <memory>
#include <optional>

namespace libcpu {

/**
 * @brief Concrete memory implementation using contiguous storage.
 * 
 * This class provides memory operations with 64-bit addressing. All methods
 * are non-virtual for performance reasons. The memory region is contiguous.
 */
class memory_staging {
public:
    /**
     * @brief Construct a new memory_staging object
     * 
     * @param mem_base Base address of the memory region
     * @param mem_size Size of memory region in bytes
     */
    memory_staging(uint64_t mem_base, size_t mem_size);

    /**
     * @brief Read from memory at the specified address.
     * 
     * @param addr The memory address to read from
     * @param width The width of the data to read
     * @param little_endian Byte ordering (true=little, false=big)
     * @return std::optional<uint64_t> Read value or std::nullopt if failed
     */
    std::optional<uint64_t> read(uint64_t addr, libvio::width_t width, bool little_endian = true);

    /**
     * @brief Write to memory at the specified address.
     * 
     * @param addr The memory address to write to
     * @param width The width of the data to write
     * @param value The value to write
     * @param little_endian Byte ordering (true=little, false=big)
     * @return bool True if write succeeded, false otherwise
     */
    bool write(uint64_t addr, libvio::width_t width, uint64_t value, bool little_endian = true);

    /**
     * @brief Get a pointer to host memory
     * 
     * @param addr Memory address to access
     * @return uint8_t* Pointer to host memory or nullptr if invalid
     */
    uint8_t* host_addr(uint64_t addr);

    /**
     * @brief Save memory contents to file
     * 
     * @param filename Output filename
     */
    void save(const char* filename) const;

    /**
     * @brief Save memory contents to stream
     * 
     * @param out Output stream
     */
    void save(std::ostream& out) const;

    /**
     * @brief Restore memory contents from file
     * 
     * @param filename Input filename
     * @return uint64_t Number of bytes loaded
     */
    uint64_t restore(const char* filename);

    /**
     * @brief Restore memory contents from stream
     * 
     * @param in Input stream
     * @return uint64_t Number of bytes loaded
     */
    uint64_t restore(std::istream& in);

    /**
     * @brief Load ELF binary (auto-detect 32/64-bit)
     * 
     * @param buffer Pointer to ELF data
     * @return uint64_t Entry point address
     */
    uint64_t load_elf(const uint8_t* buffer);

    /**
     * @brief Load ELF from file (auto-detect 32/64-bit)
     * 
     * @param filename ELF filename
     * @return uint64_t Entry point address
     */
    uint64_t load_elf_from_file(const char* filename);

    /**
     * @brief Get memory size
     * 
     * @return uint64_t Size in bytes
     */
    uint64_t get_size() const;

private:
    bool out_of_bound(uint64_t addr, libvio::width_t width) const;

    uint64_t base;
    uint64_t size;
    std::unique_ptr<uint8_t[]> mem;
};

} // namespace libcpu

#endif
