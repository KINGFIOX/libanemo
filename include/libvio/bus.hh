/**
 * @file
 * @brief Memory-mapped I/O bus system for hardware simulation
 */

#ifndef LIBVIO_BUS_HH
#define LIBVIO_BUS_HH

#include <initializer_list>
#include <libvio/frontend.hh>
#include <libvio/backend.hh>
#include <tuple>
#include <vector>
#include <memory>

namespace libvio {

/**
 * \brief Base definition for a memory-mapped I/O device
 * 
 * Represents a single MMIO device attached to a bus. Stores device's
 * address mapping and communication interfaces.
 */
class mmio_device_def {
protected:
    std::unique_ptr<io_frontend> frontend;  ///< IO frontend
    std::unique_ptr<io_backend> backend;    ///< IO backend
    uint64_t addr_begin;                    ///< Starting address in bus address space
    uint64_t byte_span;                     ///< Address range size in bytes

public:
    /**
     * @brief Construct a new MMIO device definition
     * @param frontend Frontend interface instance (ownership transferred)
     * @param backend Backend interface instance (ownership transferred)
     * @param addr_begin Starting address of device's memory map
     * @param byte_span Size of address range occupied by device
     */
    mmio_device_def(io_frontend* frontend, io_backend* backend, 
                   uint64_t addr_begin, uint64_t byte_span);
    
    friend class bus;
};

/**
 * @brief Memory-mapped I/O bus
 * 
 * Manages multiple MMIO devices connected to a shared address bus.
 * Handles address decoding and transaction routing.
 */
class bus {
public:
    /**
     * @brief Construct a bus with attached devices
     * @param device_list Initializer list of device configurations specified as tuples:
     *                    (frontend_ptr, backend_ptr, base_address, address_span)
     */
    bus(std::initializer_list<std::tuple<io_frontend*, io_backend*, uint64_t, uint64_t>> device_list);
    
    std::vector<mmio_device_def> devices;  ///< Registered devices on this bus

    /**
     * @brief Perform a read operation on the bus
     * @param addr Target address in bus address space
     * @param width Data access width
     * @return std::optional<uint64_t> Read data if successful, 
     *         std::nullopt otherwise
     */
    std::optional<uint64_t> read(uint64_t addr, width_t width);
    
    /**
     * @brief Perform a write operation on the bus
     * @param addr Target address in bus address space
     * @param width Data access width
     * @param data Data to write (least significant bits used according to width)
     * @return bool True if write succeeded, false otherwise
     */
    bool write(uint64_t addr, width_t width, uint64_t data);
    
    /**
     * @brief Advance simulation by one clock cycle
     * 
     * Propagates cycle advance to all connected devices
     */
    void next_cycle(void);
};

} // namespace libvio

#endif
