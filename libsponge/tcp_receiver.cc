#include "tcp_receiver.hh"

#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!_isn.has_value()) {
        if (seg.header().syn) {
            _isn = seg.header().seqno;
            _checkpoint = 0;
            _reassembler.push_substring(seg.payload().copy(), 0, seg.header().fin);
        }
    }

    else {
        // _checkpoint = unwrap(seg.header().seqno, _isn.value(), _checkpoint);
        auto abs_seqno = unwrap(seg.header().seqno, _isn.value(), _checkpoint);
        if (abs_seqno > _checkpoint + _capacity || abs_seqno + _capacity < _checkpoint || abs_seqno == 0)
            return;
        _checkpoint = abs_seqno;
        // cerr << "Absolute seqno: " << abs_seqno << endl;
        _reassembler.push_substring(seg.payload().copy(), _checkpoint - 1, seg.header().fin);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    return _isn.has_value() ? optional<WrappingInt32>(wrap(stream_out().bytes_written() + 1 +
                                                               static_cast<uint64_t>(stream_out().input_ended()),
                                                           _isn.value()))
                            : nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
