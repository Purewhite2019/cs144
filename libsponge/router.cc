#include "router.hh"

#include <iostream>

// #define DEBUG

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//* \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//* \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the
//route_prefix will need to match the corresponding bits of the datagram's destination address?
//* \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router
//(in which case, the next hop address should be the datagram's final destination).
//* \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);

    _routing_table.emplace_back(make_pair(route_prefix >> (32 - prefix_length), prefix_length),
                                make_pair(next_hop, interface_num));
}

//* \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    DUMMY_CODE(dgram);

    if (dgram.header().ttl == 0 || --dgram.header().ttl == 0)
        return;

    optional<decltype(_routing_table.begin())> best_it = nullopt;
    for (auto it = _routing_table.begin(); it < _routing_table.end(); ++it) {
        if (it->first.second == 0 || dgram.header().dst >> (32 - it->first.second) == it->first.first) {
            if (!best_it.has_value() || best_it.value()->first.second < it->first.second)
                best_it = it;
        }
    }

    if (best_it.has_value()) {
#ifdef DEBUG
        cerr << "DEBUG: Routing for " << dgram.header().dst << ": matched " << best_it.value()->first.first
             << " with length " << static_cast<int>(best_it.value()->first.second) << "." << endl;
        cerr << "Marching at "
             << best_it.value()->second.first.value_or(Address::from_ipv4_numeric(dgram.header().dst)).ip() << endl;
#endif  // DEBUG
        interface(best_it.value()->second.second)
            .send_datagram(dgram,
                           best_it.value()->second.first.value_or(Address::from_ipv4_numeric(dgram.header().dst)));
    } else {
#ifdef DEBUG
        cerr << "DEBUG: Routing for " << dgram.header().dst << ": no interface found, dropped." << endl;
#endif  // DEBUG
    }

    // Your code here.
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
