/**
 * @file
 * @brief Memory-mapped I/O bus system for hardware simulation
 */

#ifndef LIBVIO_BUS_HH
#define LIBVIO_BUS_HH

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <libvio/agent.hh>
#include <libvio/frontend.hh>
#include <libvio/backend.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/width.hh>
#include <optional>
#include <tuple>
#include <vector>
#include <memory>

namespace libvio {

class io_dispatcher;

class mmio_agent: public io_agent {
    public:
        std::optional<uint64_t> read(uint64_t addr, width_t width) override;
        bool write(uint64_t addr, width_t width, uint64_t data) override;
        void next_cycle(void) override;
        friend class io_dispatcher;
    private:
        io_dispatcher *dispatcher = nullptr; ///< Associated dispatcher instance
        size_t read_count = 0; ///< Count of total read requests
        size_t write_count = 0; ///< Count of total write requests
        size_t old_read_count = 0; ///< Count of total read requests before this cycle
        size_t old_write_count = 0; ///< Count of total write requests before this cycle
};

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
        mmio_device_def(io_frontend* frontend, io_backend* backend, uint64_t addr_begin, uint64_t byte_span);
        
        friend class io_dispatcher;
};

/**
 * @brief Memory-mapped I/O bus dispatcher
 * 
 * Manages multiple MMIO devices connected to a shared address bus.
 * Handles address decoding and transaction routing.
 * An `io_dispatcher` also caches the result of most recent read requests.
 * If a request with the same request number is made multiple times,
 * the cached data is returned, and the attached frontends are not invoked.
 * This is useful when more than one device under a differential test share the same virtual devices.
 */
class io_dispatcher {

    public:

        /**
        * @brief Construct a bus with attached devices
        * @param device_list Initializer list of device configurations specified as tuples:
        *                    (frontend_ptr, backend_ptr, base_address, address_span)
        * @param buffer_size Size of internal request buffer (default: 32)
        */
        io_dispatcher(std::initializer_list<std::tuple<io_frontend*, io_backend*, uint64_t, uint64_t>> device_list, size_t buffer_size=32);

        /**
        * @brief Issue a read request to the I/O bus
        *
        * The request will be routed to the device owning the specified address range.
        * If a request with the same or smaller `req_no` has been made before,
        * data cached from the ring-buffer will be returned.
        *
        * @param addr Memory address to read from
        * @param width Width of the read operation
        * @param req_no Requesst number, must start with 0 and increase 1 by a request
        * @return std::optional<uint64_t> The read data if successful, empty if failed.
        */
        std::optional<uint64_t> request_read(uint64_t addr, width_t width, size_t req_no);

        /**
        * @brief Issue a write request to the I/O bus
        *
        * The request will be routed to the device owning the specified address range.
        * If a request with the same or smaller `req_no` has been made before,
        * it will not be passed to the frontend again.
        * 
        * @param addr Memory address to write to
        * @param width Width of the write operation
        * @param req_no Request number
        * @param data Data to be written
        * @return bool True if write succeeded, false otherwise
        */
        bool request_write(uint64_t addr, width_t width, size_t req_no, uint64_t data);

        /**
         * @brief Create a new agent attached to this dispatcher
         * @return mmio_agent* Pointer to the new agent instance
         */
        mmio_agent *new_agent(void);
        
        /**
        * @brief Registered devices on this bus
        * 
        * Contains all devices attached to this bus with their configuration parameters.
        */
        std::vector<mmio_device_def> devices;

        friend class mmio_agent;

    protected:

        ringbuffer<std::tuple<uint64_t, width_t, std::optional<uint64_t>>> read_request_buffer; ///< Read request history buffer
        ringbuffer<std::tuple<uint64_t, width_t, uint64_t, bool>> write_request_buffer; ///< Write request history buffer
        std::vector<std::unique_ptr<mmio_agent>> agents; ///< Active agents attached to this dispatcher
};

} // namespace libvio

#endif
