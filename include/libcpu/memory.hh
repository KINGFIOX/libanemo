#ifndef LIBCPU_MEMORY_HH
#define LIBCPU_MEMORY_HH

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <libvio/width.hh>
#include <memory>
#include <optional>

namespace libcpu {

/**
 * @brief Abstract base class representing a view into memory.
 *
 * Provides read/write operations and memory management utilities for a specific
 * memory region. Derived classes implement the actual storage mechanism.
 */
class memory_view {
public:
    /**
     * @brief Construct a memory view from another memory view.
     *
     * This class will map the memory in `src`, from `src_base` with the size of `view_size`
     * to the address space of the new memory view from `view_base`.
     * 
     * @param src The source memory view to create a view into
     * @param src_base Base address of the new view (in the address space of source memory)
     * @param view_base Base address of the new view (in the address space of memory view)
     * @param view_size Size of the new view in bytes
     */
    memory_view(const memory_view &src, uint64_t src_base, uint64_t view_base, uint64_t view_size);

    /**
     * @brief Read data from memory.
     * 
     * @param addr The memory address to read from
     * @param width The width of the data to read
     * @param little_endian Byte ordering (true for little-endian, false for big-endian)
     * @return The read value, or std::nullopt if read failed
     */
    std::optional<uint64_t> read(uint64_t addr, libvio::width_t width, bool little_endian = true);

    /**
     * @brief Write data to memory.
     * 
     * @param addr The memory address to write to
     * @param width The width of the data to write (1-8 bytes)
     * @param value The value to write
     * @param little_endian Byte ordering (true for little-endian, false for big-endian)
     * @return true if write succeeded, false if failed
     */
    bool write(uint64_t addr, libvio::width_t width, uint64_t value, bool little_endian = true);

    /**
     * @brief Get a direct pointer to host memory.
     * 
     * @param addr Memory address to access
     * @return Pointer to host memory at the specified address,
     *                  or nullptr if address is invalid
     */
    uint8_t* host_addr(uint64_t addr);

    /**
     * @brief Save memory contents to a file.
     * 
     * @param filename Path to the output file
     */
    void save(const char* filename) const;

    /**
     * @brief Save memory contents to an output stream.
     * 
     * @param out Output stream to write to
     */
    void save(std::ostream& out) const;

    /**
     * @brief Restore memory contents from a file.
     * 
     * @param filename Path to the input file
     * @return Number of bytes successfully loaded
     */
    uint64_t restore(const char* filename);

    /**
     * @brief Restore memory contents from an input stream.
     * 
     * @param in Input stream to read from
     * @return Number of bytes successfully loaded
     */
    uint64_t restore(std::istream& in);

    /**
     * @brief Load an ELF binary into memory (auto-detects 32/64-bit format).
     * 
     * @param buffer Pointer to the ELF file data in memory
     * @return  The entry point address of the loaded ELF
     */
    uint64_t load_elf(const uint8_t* buffer);

    /**
     * @brief Load an ELF file into memory (auto-detects 32/64-bit format).
     * 
     * @param filename Path to the ELF file
     * @return The entry point address of the loaded ELF
     */
    uint64_t load_elf_from_file(const char* filename);

    /**
     * @brief Get the size of the memory region.
     * 
     * @return uint64_t Size of the memory region in bytes
     */
    uint64_t get_size() const;

    /**
     * @brief Check if a memory access would be out of bounds.
     *
     * @param addr The address to check
     * @param width The width of the memory access
     * @return bool True if the address range [addr, addr+width-1] is invalid,
     *              false otherwise
     */
    bool out_of_bound(uint64_t addr, libvio::width_t width) const;

protected:
    uint8_t *mem_ptr;  ///< Pointer to the memory storage
    uint64_t base;     ///< Base address of the memory region
    uint64_t size;     ///< Size of the memory region in bytes

    /**
     * @brief Protected default constructor for derived classes.
     */
    memory_view();
};

/**
 * @brief Concrete memory implementation using contiguous storage.
 * 
 * This class provides memory operations with 64-bit addressing using a
 * contiguous block of memory. All methods are non-virtual for performance.
 */
class memory: public memory_view {
public:
    /**
     * @brief Construct a new memory object with contiguous storage.
     * 
     * @param mem_base Base address of the memory region
     * @param mem_size Size of memory region in bytes
     */
    memory(uint64_t mem_base, size_t mem_size);

protected:
    std::unique_ptr<uint8_t[]> mem;  ///< Contiguous memory storage
};

} // namespace libcpu

#endif
