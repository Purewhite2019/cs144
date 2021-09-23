#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::_send() {
    while (!_sender.segments_out().empty()) {
        // cerr << "Send a normal packet, length: " << _sender.segments_out().front().length_in_sequence_space() <<
        // endl;
        auto ackno = _receiver.ackno();
        if (ackno.has_value()) {
            _sender.segments_out().front().header().ack = true;
            _sender.segments_out().front().header().ackno = ackno.value();
        }
        _sender.segments_out().front().header().win = _receiver.window_size();
        // cerr << "Send: " << _sender.segments_out().front().payload().size() << " : " <<
        // _sender.segments_out().front().payload().str() << endl;
        _segments_out.emplace(move(_sender.segments_out().front()));
        _sender.segments_out().pop();
    }
}

void TCPConnection::_send_reset() {
    _sender.send_empty_segment();
    _sender.segments_out().front().header().rst = true;
    // cerr << "Send Reset: " << _sender.segments_out().front().payload().size() << " : " <<
    // _sender.segments_out().front().payload().str() << endl;
    _segments_out.emplace(move(_sender.segments_out().front()));
    _sender.segments_out().pop();
}

void TCPConnection::_reset() {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _running = false;  // TODO: Reconsider if we really need it
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment_received = 0;
    if (seg.header().rst) {
        // cerr << "Connection: Passive reset" << endl;
        _reset();
        return;
    }
    _receiver.segment_received(seg);
    if (_sender.next_seqno_absolute() > 0 || seg.header().syn) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        _sender.fill_window();
        if (_sender.segments_out().empty() && seg.length_in_sequence_space() > 0) {
            _sender.send_empty_segment();
        }
        _send();
    }
    if (inbound_stream().input_ended() && !_sender.stream_in().eof())
        _linger_after_streams_finish = false;
}

bool TCPConnection::active() const { return _running; }

size_t TCPConnection::write(const string &data) {
    size_t write_count = _sender.stream_in().write(data);
    _sender.fill_window();
    _send();
    return write_count;
}

//* \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (_running) {
        // cerr << "Tick: " << ms_since_last_tick << " ms passed" << endl;
        _time_since_last_segment_received += ms_since_last_tick;
        if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && inbound_stream().eof()) {
            if ((_linger_after_streams_finish && _time_since_last_segment_received >= 10 * _cfg.rt_timeout) ||
                _linger_after_streams_finish == false)
                _running = false;
        }
        if (_sender.consecutive_retransmissions() >= _cfg.MAX_RETX_ATTEMPTS) {
            // cerr << "consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS: Reset." << endl;
            // cerr << "Connection: Active reset" << endl;
            _send_reset();
            _reset();
            return;
        }
        _sender.tick(ms_since_last_tick);
        _send();
        // cerr << "End tick." << endl;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    _send();
}

void TCPConnection::connect() {
    _sender.fill_window();
    _send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _reset();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
