#include <cstdint>
#include <libvio/frontend.hh>
#include <libvio/console.hh>

namespace libvio {

ioreq_t console_frontend::resolve_read(uint64_t offset, width_t width) const {
    if (offset==0 && width==width_t::byte) {
        // receiving
        return {ioreq_type_t::read, reqval::console_rx};
    } else if (offset==1 && width==width_t::byte) {
        // querying device state
        // bit 0: output ready
        // bit 1: input valid
        return {ioreq_type_t::ioctl_get, reqval::console_rx|reqval::console_tx};
    } else {
        return {ioreq_type_t::invalid, 0};
    }
}

ioreq_t console_frontend::resolve_write(size_t offset, width_t width, uint64_t data) const {
    if (offset==0 && width==width_t::byte) {
        // sending
        return {ioreq_type_t::write, reqval::console_tx};
    } else if (offset==2 && width==width_t::half) {
        // trying to set prescaler
        // ignore this
        return {ioreq_type_t::ioctl_set, reqval::console_prescaler};
    } else {
        return {ioreq_type_t::invalid, 0};;
    }
}

uint64_t console_frontend::ioctl_get(uint64_t req) {
    // tx is always ready when using software emulated console
    uint64_t tx_ready = 1;
    uint64_t rx_valid = backend->poll(reqval::console_rx);
    return (rx_valid<<1) | (tx_ready<<0);
}

void console_frontend::ioctl_set(uint64_t req, uint64_t data) {
    return;
}

}