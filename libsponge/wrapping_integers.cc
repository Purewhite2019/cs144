#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//* Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//* \param n The input absolute 64-bit sequence number
//* \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return isn + static_cast<uint32_t>(n); }

//* Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//* \param n The relative sequence number
//* \param isn The initial sequence number
//* \param checkpoint A recent absolute 64-bit sequence number
//* \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//*
//* \note Each of the two streams of the TCP connection has its own ISN. One stream
//* runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//* and the other stream runs from the remote TCPSender to the local TCPReceiver and
//* has a different ISN.

// uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
//     int64_t margin = n.raw_value() - isn.raw_value();
//     int64_t cmp = margin - (checkpoint & 0xffffffff);

//     if (cmp < -(1l<<31))
//         margin += (1l<<32);
//     else if(cmp > (1l<<31))
//         margin -= (1l<<32);

//     if (!(margin < 0 && (checkpoint & 0xffffffff00000000) < static_cast<uint64_t>(-margin)))
//         return (checkpoint & 0xffffffff00000000) + margin;
//     else
//         return (checkpoint & 0xffffffff00000000) + static_cast<uint32_t>(margin);
// }

//* A simpler method inspired by a masterpiece from the Internet
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    auto steps = n - wrap(checkpoint, isn);
    return (steps < 0 && checkpoint < static_cast<uint64_t>(-steps)) ? checkpoint + static_cast<uint32_t>(steps)
                                                                     : checkpoint + steps;
}