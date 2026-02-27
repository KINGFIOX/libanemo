#ifndef LIBVIO_AGENT_HH
#define LIBVIO_AGENT_HH

#include <optional>
#include <cstdint>
#include <libvio/width.hh>

namespace libvio {

/**
 * @brief Provides am MMIO interface for simulated processors.
 */
class io_agent{
    public:
        /**
        * @brief Perform a read operation on the bus
        * @param addr Target address in bus address space
        * @param width Data access width
        * @return std::optional<uint64_t> Read data if successful, std::nullopt otherwise
        */
        virtual std::optional<uint64_t> read(uint64_t addr, width_t width) = 0;
        
        /**
        * @brief Perform a write operation on the bus
        * @param addr Target address in bus address space
        * @param width Data access width
        * @param data Data to write (least significant bits used according to width)
        * @return `true` if write succeeded, `false` otherwise
        */
        virtual bool write(uint64_t addr, width_t width, uint64_t data) = 0;
};

}

#endif
