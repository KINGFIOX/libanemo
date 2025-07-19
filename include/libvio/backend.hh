#ifndef LIBVIO_BACKEND_HH
#define LIBVIO_BACKEND_HH

#include <cstdint>

namespace libvio { 

/**
 * @brief Abstract base class for I/O backends
 * 
 * This class does the actual IO operations. It also acts as a producer of input data.
 * Each member function requires an argument `req`, whose meaning is defined by subclasses.
 * The behavior is undefined if `req` is invalid.
 * @see namespace `libvio::reqval` for `req` values of each subclass
 */
class io_backend {
    public:
        /**
        * @brief Blocking read request
        * 
        * Retrieves input data. Blocks until data becomes available.
        * This function is used by the frontends when the processor explicitly reads input via MMIO.
        * The blocking behavior makes sure that simple programs assuming the input data is always available will work.
        * 
        * @param req MMIO operation description
        * @return uint64_t The input data
        */
        virtual uint64_t request(uint64_t req) = 0;

        /**
        * @brief Blocking input availability check
        * 
        * Checks if input data is available. This may block if data is not immediately available,
        * This interface is used by the frontend when the processor expilcitly checks whether input is available via MMIO.
        * Some backends are synchronous, they must block to wait for data.
        * If they do not block and return with "not available", the processor might think the device is busy and will never do an input.
        * 
        * @param req MMIO operation description
        * @return whether the requested input data is available
        */
        virtual bool poll(uint64_t req) = 0;

        /**
        * @brief Non-blocking input availability check
        * 
        * Check whether input data is available. This never blocks.
        * This interface is used by the frontend when the frontend itself needs to know whether the input is available.
        * 
        * @param req MMIO operation description
        * @return whether requested input data is currently available
        */
        virtual bool check(uint64_t req) = 0;

        /**
        * @brief Non-blocking write
        * 
        * Sends output data to the backend. This never blocks.
        * 
        * @param req MMIO operation description
        * @param data Output data
        */
        virtual void put(uint64_t req, uint64_t data) = 0;

        virtual ~io_backend() = default;
};

}

#endif