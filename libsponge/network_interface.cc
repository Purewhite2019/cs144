#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void NetworkInterface::update(const uint32_t ip, const EthernetAddress &eaddr) {
    auto it = _map_ip_eth.find(ip);
    if (it == _map_ip_eth.end()) {
        _map_ip_eth.emplace(ip, make_pair(eaddr, NetworkInterface::LIVETIME_ARP));
    } else {
        it->second.first = eaddr;
        it->second.second = NetworkInterface::LIVETIME_ARP;
    }

    auto q = _queue_dgram.find(ip);
    if (q != _queue_dgram.end()) {
        while (!q->second.empty()) {
            EthernetFrame eframe;
            eframe.header().dst = eaddr;
            eframe.header().src = _ethernet_address;
            eframe.header().type = EthernetHeader::TYPE_IPv4;
            eframe.payload() = q->second.front().serialize();
            frames_out().emplace(eframe);

            q->second.pop();
        }
        _queue_dgram.erase(q);
    }
}

//* \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//* \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//* \param[in] dgram the IPv4 datagram to be sent
//* \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may
// also be another host if directly connected to the same network as the destination)
//* (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric()
// method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    auto it = _map_ip_eth.find(next_hop_ip);
    if (it == _map_ip_eth.end()) {  // Not cached and ARP not sent or ARP sent before 5 seconds ago.
        auto dit = _queue_dgram.find(next_hop_ip);
        if (dit == _queue_dgram.end()) {
            auto nit = _queue_dgram.emplace(next_hop_ip, std::queue<InternetDatagram>{});
            nit.first->second.push(dgram);
        } else
            dit->second.push(dgram);

        ARPMessage arp;
        arp.opcode = ARPMessage::OPCODE_REQUEST;
        arp.sender_ethernet_address = _ethernet_address;
        arp.sender_ip_address = _ip_address.ipv4_numeric();
        arp.target_ethernet_address = {};  // TODO: Check is this okay?
        arp.target_ip_address = next_hop_ip;

        EthernetFrame eframe;
        eframe.header().dst = ETHERNET_BROADCAST;
        eframe.header().src = _ethernet_address;
        eframe.header().type = EthernetHeader::TYPE_ARP;
        eframe.payload() = arp.serialize();
        frames_out().emplace(eframe);

        _map_ip_eth.emplace(next_hop_ip, make_pair(std::nullopt, TIMEOUT_ARP));
    } else {
        if (it->second.first.has_value()) {  // Already cached.
            EthernetFrame eframe;
            eframe.header().dst = it->second.first.value();
            eframe.header().src = _ethernet_address;
            eframe.header().type = EthernetHeader::TYPE_IPv4;
            eframe.payload() = dgram.serialize();
            frames_out().emplace(eframe);
        } else {  // ARP sent in the last 5 seconds
            _queue_dgram[next_hop_ip].push(dgram);
        }
    }
    DUMMY_CODE(dgram, next_hop, next_hop_ip);
}

//* \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST)
        return std::nullopt;
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        auto result = dgram.parse(frame.payload());
        if (result == ParseResult::NoError)
            return dgram;
        else
            return std::nullopt;
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;
        auto result = arp.parse(frame.payload());
        if (result == ParseResult::NoError) {
            update(arp.sender_ip_address, arp.sender_ethernet_address);
            if (arp.opcode == ARPMessage::OPCODE_REQUEST) {
                if (arp.target_ip_address == _ip_address.ipv4_numeric()) {
                    ARPMessage reply;
                    reply.opcode = ARPMessage::OPCODE_REPLY;
                    reply.sender_ethernet_address = _ethernet_address;
                    reply.sender_ip_address = _ip_address.ipv4_numeric();
                    reply.target_ethernet_address = arp.sender_ethernet_address;
                    reply.target_ip_address = arp.sender_ip_address;

                    EthernetFrame eframe;
                    eframe.header().dst = arp.sender_ethernet_address;
                    eframe.header().src = _ethernet_address;
                    eframe.header().type = EthernetHeader::TYPE_ARP;
                    eframe.payload() = reply.serialize();
                    frames_out().emplace(eframe);

                    _map_ip_eth.emplace(arp.sender_ip_address, make_pair(std::nullopt, TIMEOUT_ARP));
                }
            } else if (arp.opcode == ARPMessage::OPCODE_REPLY) {
                // update(arp.target_ip_address, arp.target_ethernet_address); // As the document requires, only
                // remember the sender.
            } else {
                // throw runtime_error("Invalid ARP OPCode: " + std::to_string(arp.opcode));
            }
        }
    } else {
        // throw runtime_error("Unexpected frame type: " + std::to_string(frame.header().type));
    }

    return nullopt;
}

//* \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    DUMMY_CODE(ms_since_last_tick);

    auto it = _map_ip_eth.begin();
    while (it != _map_ip_eth.end()) {
        if (it->second.second <= ms_since_last_tick) {
            it = _map_ip_eth.erase(it);
        } else {
            it->second.second -= ms_since_last_tick;
            ++it;
        }
    }
}
