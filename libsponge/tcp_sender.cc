#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
// #include <iostream>
// using std::cerr;
// using std::endl;

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//* \param[in] capacity the capacity of the outgoing byte stream
//* \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//* \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timer(_initial_retransmission_timeout, this) {}

uint64_t TCPSender::bytes_in_flight() const {
    return _segments_buffer.empty() ? 0
                                    : _next_seqno - unwrap(_segments_buffer.front().header().seqno, _isn, _next_seqno);
}

void TCPSender::fill_window() {
    while (true) {
        if (next_seqno_absolute() > 0 && next_seqno_absolute() == bytes_in_flight()) {  // SYN Sent
            // cerr << "None sent. Waiting for ACK for SYN" << endl;
            return;
        }
        if (stream_in().eof() &&
            next_seqno_absolute() == stream_in().bytes_written() + 2) {  // FIN Sent and following states
            // cerr << "None sent. Fin sent" << endl;
            return;
        }
        size_t remaining_capacity = max(static_cast<uint64_t>(window_size()), 1ul) - bytes_in_flight();
        if (max(static_cast<uint64_t>(window_size()), 1ul) <= bytes_in_flight()) {
            // cerr << "None sent. Window is full" << endl;
            return;
        }
        size_t str_size = min(remaining_capacity - (_next_seqno == 0), stream_in().buffer_size());

        TCPSegment seg;
        seg.payload() = stream_in().read(min(str_size, TCPConfig::MAX_PAYLOAD_SIZE));
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().syn = (_next_seqno == 0);
        seg.header().fin = stream_in().input_ended() && stream_in().buffer_empty() &&
                           seg.payload().size() + seg.header().syn + 1 <= remaining_capacity;

        if (seg.length_in_sequence_space() == 0) {
            // cerr << "None sent. Null seg." << endl;
            return;
        }
        _segments_buffer.push(seg);
        _segments_out.push(seg);
        // cerr << "Push segment: " << _segments_out.back().payload().size() << ' ' << (seg.header().syn ? "SYN " : "")
        // << (seg.header().fin ? "FIN " : "") << " : " << _segments_out.back().payload().str() << endl; cerr << "Sent "
        // << seg.payload().str().size() << ' ' << (seg.header().syn ? "SYN " : "") << (seg.header().fin ? "FIN " : "")
        // << "at " << _next_seqno; cerr << " cont: " << (seg.payload().str().size() < 10 ? seg.payload().str() :
        // seg.payload().str().substr(0, 10));
        _next_seqno += seg.length_in_sequence_space();
        if (seg.header().fin) {
            break;
        }
        // cerr << " next " << _next_seqno << endl;
    }
}

//* \param ackno The remote receiver's ackno (acknowledgment number)
//* \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size = window_size;
    auto abs_ackno = unwrap(ackno, _isn, _next_seqno);
    if (abs_ackno > _next_seqno)
        return;
    // cerr << "Ack Received: " << abs_ackno << ", window size: " << window_size << " nwd: " << _window_size << endl;
    if (!_segments_buffer.empty() && (unwrap(_segments_buffer.front().header().seqno, _isn, _next_seqno) +
                                          _segments_buffer.front().length_in_sequence_space() <=
                                      abs_ackno)) {
        _retransmission_timer.set(_initial_retransmission_timeout);

        do {
            // cerr << "Acked seg " << unwrap(_segments_buffer.front().header().seqno, _isn, _next_seqno) << endl;
            _segments_buffer.pop();
        } while (!_segments_buffer.empty() && (unwrap(_segments_buffer.front().header().seqno, _isn, _next_seqno) +
                                                   _segments_buffer.front().length_in_sequence_space() <=
                                               abs_ackno));
    }
}

//* \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_segments_buffer.empty()) {
        _retransmission_timer.tick(ms_since_last_tick);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_timer.retransmission_count(); }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    // _segments_buffer.push(seg);
    // cerr << "Push empty segment. " << endl;
    _segments_out.push(seg);
    // cerr << "Sent Empty " << _next_seqno << endl;
}

void TCPSender::try_retransmit() {
    // _segments_buffer.push(_segments_buffer.front());
    if (!_segments_buffer.empty()) {
        // cerr << "Try retransmitting: " << _segments_buffer.front().payload().size() << " : " <<
        // _segments_buffer.front().payload().str() << endl;
        _segments_out.push(_segments_buffer.front());
    }
    // cerr << "Resent " << unwrap(_segments_buffer.front().header().seqno, _isn, _next_seqno) << endl;
}

uint16_t TCPSender::window_size() { return _window_size; }

void TCPSender::RetransmissionTimer::tick(const size_t ms_since_last_tick) {
    _current_time += ms_since_last_tick;
    if (_current_time >= _retransmission_timeout) {
        _sender->try_retransmit();
        if (_sender->window_size() != 0) {  // ! Why should window_size() != 0?
            ++_retransmission_count;
            exp_backoff();
        }
        _current_time = 0;
    }
}
void TCPSender::RetransmissionTimer::set(const unsigned int retransmission_timeout) {
    _retransmission_timeout = retransmission_timeout;
    reset();
}
void TCPSender::RetransmissionTimer::reset() {
    _current_time = 0;
    _retransmission_count = 0;
}
void TCPSender::RetransmissionTimer::exp_backoff() { _retransmission_timeout *= 2; }