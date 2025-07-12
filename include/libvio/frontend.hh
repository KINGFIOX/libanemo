/**
 * @file frontend.hh
 * @brief I/O Frontend interface definition for libvio
 */
#ifndef LIBVIO_FRONTEND_HH
#define LIBVIO_FRONTEND_HH

#include <libvio/width.hh>
#include <libvio/backend.hh>
#include <cstdint>
#include <optional>

namespace libvio {

/**
 * @enum ioreq_type_t
 * @brief Type of I/O request being made
 */
enum class ioreq_type_t {
    read,       ///< Memory read operation
    write,      ///< Memory write operation
    poll_in,    ///< Input availability check
    poll_out,   ///< Output readiness check
    ioctl_get,  ///< Get control parameter
    ioctl_set,  ///< Set control parameter
    invalid     ///< Invalid/uninitialized request
};

/**
 * @struct ioreq_t
 * @brief Encapsulates a resolved I/O request
 * @see namespace `libvio::req` for `req` values of each type of device
 */
struct ioreq_t {
    ioreq_type_t type;  ///< Type of I/O operation requested
    uint64_t req;       ///< Frontend-specific request identifier/parameter
};

/**
 * @class io_frontend
 * @brief Abstract base class for I/O frontend implementations
 * 
 * This classes calls the backend on the first read or write request in a cycle.
 * The result is cached and used for subsequent requests.
 * This ensures each CPU in a difftest gets the same MMIO result.
 * And the same MMIO operation is not excuted more than once.
 * This class assumes that a CPU does not generate more than one MMIO request to the same device in a cycle.
 * There will be an error if there are different read / write requests in the same cycle.
 * Derived classes must implement request resolution logic.
 */
class io_frontend {
public:
    io_backend *backend;  ///< Associated backend for I/O operations

    /**
     * @brief Resolve a read request to backend-specific operation
     * @param offset Memory address offset
     * @param width Data access width
     * @return Resolved I/O request structure
     */
    virtual ioreq_t resolve_read(uint64_t offset, width_t width) const = 0;

    /**
     * @brief Resolve a write request to backend-specific operation
     * @param offset Memory address offset
     * @param width Data access width
     * @param data Data to be written
     * @return Resolved I/O request structure
     */
    virtual ioreq_t resolve_write(uint64_t offset, width_t width, uint64_t data) const = 0;

    /**
     * @brief Execute read operation (with cycle caching)
     * @param offset Memory address offset
     * @param width Data access width
     * @return Read data or nullopt if request fails
     */
    virtual std::optional<uint64_t> read(uint64_t offset, width_t width);

    /**
     * @brief Execute write operation (with cycle caching)
     * @param offset Memory address offset
     * @param width Data access width
     * @param data Data to be written
     * @return true if write succeeded, false otherwise
     */
    virtual bool write(uint64_t offset, width_t width, uint64_t data);

    /**
     * @brief Advance to next simulation cycle
     * 
     * Clears cached requests and results. Must be called
     * at each cycle boundary.
     */
    virtual void next_cycle(void);

protected:
    /**
     * @brief Cached read address for current cycle
     * 
     * Subsequent reads to same address use cached data
     */
    std::optional<uint64_t> read_offset = {};

    /**
     * @brief Cached read data for current cycle
     * 
     * Valid when read_offset has value
     */
    std::optional<uint64_t> read_data;

    /**
     * @brief Cached write address for current cycle
     * 
     * Subsequent writes to same address are ignored
     */
    std::optional<uint64_t> write_offset = {};

    uint64_t write_data;  ///< Cached write data for current cycle
    bool write_result;    ///< Cached write status for current cycle

    /**
     * @brief Handle control parameter get operation
     * @param req Control request identifier
     * @return Current parameter value
     * 
     * Called automatically by read() for ioctl_get requests
     */
    virtual uint64_t ioctl_get(uint64_t req);

    /**
     * @brief Handle control parameter set operation
     * @param req Control request identifier
     * @param value New parameter value
     * 
     * Called automatically by write() for ioctl_set requests.
     */
    virtual void ioctl_set(uint64_t req, uint64_t value);
};

}

#endif
