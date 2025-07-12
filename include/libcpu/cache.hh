#ifndef LIBCPU_CACHE_HH
#define LIBCPU_CACHE_HH

#include <cstddef>
#include <vector>
#include <memory>
#include <cstdint>
#include <optional>
#include <fstream>
#include <libcpu/memory.hh>

namespace libcpu {

/**
 * @brief Abstract base class for cache implementations
 * 
 * @tparam WORD_T The type for address and data. Must be an unsigned integral type.
 * 
 * This class extends abstract_memory with cache-specific functionality including
 * invalidation and performance statistics tracking.
 */
template <typename WORD_T>
class abstract_cache : public abstract_memory<WORD_T> {
public:
    abstract_memory<WORD_T>* underlying_memory;  // Public pointer to underlying memory

    /**
     * @brief Invalidate a specific cache line
     * @param addr Address within the cache line to invalidate
     */
    virtual void invalidate(WORD_T addr) = 0;
    /**
     * @brief Invalidate all cache lines
     */
    virtual void invalidate() = 0;
    
    /**
     * @brief Get total cache hits
     * @return Number of successful cache hits since creation or last reset
     */
    virtual uint64_t hits() const = 0;
    /**
     * @brief Get total cache misses
     * @return Number of cache misses since creation or last reset
     */
    virtual uint64_t misses() const = 0;

    // Debugging functions bypass cache
    std::optional<WORD_T> peek(WORD_T addr, libvio::width_t width, bool little_endian = true) const override {
        return underlying_memory->peek(addr, width, little_endian);
    }

    bool set(WORD_T addr, libvio::width_t width, WORD_T value, bool little_endian = true) override {
        return underlying_memory->set(addr, width, value, little_endian);
    }

    uint8_t* host_addr(WORD_T addr) override {
        return underlying_memory->host_addr(addr);
    }

    // Save/restore must be implemented by concrete caches
    void save(const char* filename) const override = 0;
    WORD_T restore(const char* filename) override = 0;
};

/**
 * @brief Direct-mapped write-through cache implementation
 * 
 * @tparam WORD_T The type for address and data. Must be an unsigned integral type.
 * @tparam OFFSET_BITS Number of bits used for block offset (determines block size)
 * @tparam INDEX_BITS Number of bits used for index (determines number of blocks)
 * 
 * @note This class is only a demo of the cache interface. It is not completly tested or optimized for performance.
 */
template <typename WORD_T, uint_fast8_t OFFSET_BITS, uint_fast8_t INDEX_BITS>
class direct_cache: public abstract_cache<WORD_T> {
    public:
        static constexpr uint_fast8_t offset_bits = OFFSET_BITS;
        static constexpr uint_fast8_t index_bits = INDEX_BITS;
        static constexpr WORD_T tag_bits = sizeof(WORD_T) * 8 - index_bits - offset_bits;
        static constexpr WORD_T tag_mask = (size_t(1) << tag_bits) - 1;
        static constexpr size_t num_lines = size_t(1) << index_bits;
        static constexpr size_t block_size = size_t(1) << offset_bits;

    static_assert(std::is_unsigned_v<WORD_T>, "WORD_T must be unsigned type");
    static_assert(offset_bits > 0 && offset_bits < sizeof(WORD_T)*8, 
                 "Invalid offset_bits");

private:
    std::unique_ptr<uint8_t[]> data;
    std::unique_ptr<bool[]> valid;
    std::unique_ptr<WORD_T[]> tags;

    uint64_t hit_count = 0;
    uint64_t miss_count = 0;

    // Address decomposition
    struct cache_address {
        WORD_T tag;
        WORD_T index;
        WORD_T offset;
    };

    cache_address decompose(WORD_T addr) const {
        WORD_T offset = addr & (block_size - 1);
        WORD_T index = (addr >> offset_bits) & (num_lines - 1);
        WORD_T tag = (addr >> (offset_bits + index_bits)) & tag_mask;
        return {tag, index, offset};
    }

    // Check if access is within a single block
    bool within_block(WORD_T addr, libvio::width_t width) const {
        WORD_T start = addr;
        WORD_T end = addr + static_cast<WORD_T>(width) - 1;
        return (start >> offset_bits) == (end >> offset_bits);
    }

    // Fetch block from underlying memory
    void fetch_block(WORD_T block_base) {
        ++miss_count;
        auto dec = decompose(block_base);
        uint8_t* block_ptr = data.get() + dec.index * block_size;

        if (uint8_t* underlying_ptr = this->underlying_memory->host_addr(block_base)) {
            std::copy(underlying_ptr, underlying_ptr + block_size, block_ptr);
        } else {
            for (WORD_T i = 0; i < block_size; ++i) {
                auto byte_val = this->underlying_memory->peek(block_base + i, 
                                                             libvio::width_t::byte);
                if (!byte_val) {
                    valid[dec.index] = false;
                    return;
                }
                block_ptr[i] = static_cast<uint8_t>(*byte_val);
            }
        }
        tags[dec.index] = dec.tag;
        valid[dec.index] = true;
    }

    // Read from cache block
    std::optional<WORD_T> read_block(WORD_T addr, libvio::width_t width, 
                                     bool little_endian) {
        auto dec = decompose(addr);
        if (!valid[dec.index] || tags[dec.index] != dec.tag) {
            fetch_block(addr - dec.offset);
        } else {
            hit_count++;  // Count cache hit
        }

        uint8_t* block_ptr = data.get() + dec.index * block_size + dec.offset;
        size_t w = static_cast<size_t>(width);
        WORD_T value = 0;

        if (little_endian) {
            for (size_t i = 0; i < w; ++i) {
                value |= static_cast<WORD_T>(block_ptr[i]) << (i * 8);
            }
        } else {
            for (size_t i = 0; i < w; ++i) {
                value = (value << 8) | block_ptr[i];
            }
        }
        return value;
    }

    // Write to cache block
    bool write_block(WORD_T addr, libvio::width_t width, 
                     WORD_T value, bool little_endian) {
        auto dec = decompose(addr);
        if (!valid[dec.index] || tags[dec.index] != dec.tag) {
            fetch_block(addr - dec.offset);
        } else {
            hit_count++;  // Count cache hit
        }

        uint8_t* block_ptr = data.get() + dec.index * block_size + dec.offset;
        size_t w = static_cast<size_t>(width);

        if (little_endian) {
            for (size_t i = 0; i < w; ++i) {
                block_ptr[i] = (value >> (i * 8)) & 0xFF;
            }
        } else {
            for (size_t i = 0; i < w; ++i) {
                block_ptr[i] = (value >> ((w - 1 - i) * 8)) & 0xFF;
            }
        }

        // Write-through to underlying memory
        return this->underlying_memory->write(addr, width, value, little_endian);
    }

public:
    direct_cache() {
        // Allocate storage
        data = std::make_unique<uint8_t[]>(num_lines * block_size);
        valid = std::make_unique<bool[]>(num_lines);
        tags = std::make_unique<WORD_T[]>(num_lines);
        
        // Initialize cache state
        for (uint64_t i = 0; i < num_lines; ++i) {
            valid[i] = false;
        }
    }

    // Memory interface functions
    std::optional<WORD_T> read(WORD_T addr, libvio::width_t width, 
                               bool little_endian = true) override {
        if (within_block(addr, width)) {
            return read_block(addr, width, little_endian);
        }

        // Handle unaligned access
        WORD_T block_mask = ~(block_size - 1);
        WORD_T block_end = (addr & block_mask) + block_size;
        WORD_T first_part_size = block_end - addr;
        WORD_T second_addr = block_end;
        WORD_T second_part_size = static_cast<WORD_T>(width) - first_part_size;

        auto part1 = read_block(addr, static_cast<libvio::width_t>(first_part_size), 
                               little_endian);
        auto part2 = read_block(second_addr, static_cast<libvio::width_t>(second_part_size), 
                               little_endian);

        if (!part1 || !part2) return std::nullopt;

        if (little_endian) {
            return *part1 | (*part2 << (first_part_size * 8));
        } else {
            return (*part1 << (second_part_size * 8)) | *part2;
        }
    }

    bool write(WORD_T addr, libvio::width_t width, WORD_T value, 
               bool little_endian = true) override {
        if (within_block(addr, width)) {
            return write_block(addr, width, value, little_endian);
        }

        // Handle unaligned access
        WORD_T block_mask = ~(block_size - 1);
        WORD_T block_end = (addr & block_mask) + block_size;
        WORD_T first_part_size = block_end - addr;
        WORD_T second_addr = block_end;
        WORD_T second_part_size = static_cast<WORD_T>(width) - first_part_size;

        WORD_T part1, part2;
        if (little_endian) {
            part1 = value & ((1ULL << (first_part_size * 8)) - 1);
            part2 = value >> (first_part_size * 8);
        } else {
            part1 = value >> (second_part_size * 8);
            part2 = value & ((1ULL << (second_part_size * 8)) - 1);
        }

        bool success1 = write_block(addr, static_cast<libvio::width_t>(first_part_size), 
                                   part1, little_endian);
        bool success2 = write_block(second_addr, static_cast<libvio::width_t>(second_part_size), 
                                   part2, little_endian);
        return success1 && success2;
    }

    // Invalidation functions
    void invalidate(WORD_T addr) override {
        auto dec = decompose(addr);
        valid[dec.index] = false;
    }

    void invalidate() override {
        for (uint64_t i = 0; i < num_lines; ++i) {
            valid[i] = false;
        }
    }

    // Hit/miss statistics accessors
    uint64_t hits() const override { return hit_count; }
    uint64_t misses() const override { return miss_count; }

    // Cache state management
    void save(const char* filename) const override {
        std::ofstream out(filename, std::ios::binary);
        if (!out) return;

        // Write header (num_lines and block_size)
        uint64_t header[2] = {hit_count, miss_count};
        out.write(reinterpret_cast<const char*>(header), sizeof(header));

        // Write valid flags
        std::vector<uint8_t> valid_bytes(num_lines);
        for (uint64_t i = 0; i < num_lines; ++i) {
            valid_bytes[i] = valid[i] ? 1 : 0;
        }
        out.write(reinterpret_cast<const char*>(valid_bytes.data()), num_lines);

        // Write tags
        out.write(reinterpret_cast<const char*>(tags.get()), num_lines * sizeof(WORD_T));

        // Write data
        out.write(reinterpret_cast<const char*>(data.get()), num_lines * block_size);
    }

    WORD_T restore(const char* filename) override {
        std::ifstream in(filename, std::ios::binary);
        if (!in) return 0;

        // Read header
        uint64_t header[2];
        in.read(reinterpret_cast<char*>(header), sizeof(header));
        hit_count = header[0];
        miss_count = header[1];

        // Read valid flags
        std::vector<uint8_t> valid_bytes(num_lines);
        in.read(reinterpret_cast<char*>(valid_bytes.data()), num_lines);
        for (uint64_t i = 0; i < num_lines; ++i) {
            valid[i] = (valid_bytes[i] != 0);
        }

        // Read tags
        in.read(reinterpret_cast<char*>(tags.get()), num_lines * sizeof(WORD_T));

        // Read data
        in.read(reinterpret_cast<char*>(data.get()), num_lines * block_size);

        return num_lines * block_size;  // Return bytes read
    }
};

} // namespace libcpu

#endif
