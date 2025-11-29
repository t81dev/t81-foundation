/**
 * @file T81Discovery.hpp
 * @brief Defines a zero-configuration peer discovery protocol for T81 agents.
 *
 * This file contains the implementation of the T81Discovery class, which allows
 * T81 agents to find each other on a local network. It uses UDP broadcasting
 * to send out "beacons" containing the agent's identity and listening port.
 * The protocol is designed to be zero-configuration and includes entropy-based
 * signatures to verify the authenticity of beacons.
 */
#pragma once

#include "t81/T81Network.hpp"
#include "t81/T81Agent.hpp"
#include "t81/T81Time.hpp"
#include "t81/T81Entropy.hpp"
#include "t81/T81Bytes.hpp"
#include "t81/T81Symbol.hpp"
#include <asio.hpp>
#include <thread>
#include <set>
#include <mutex>

namespace t81 {
using asio::ip::udp;

// ======================================================================
// T81Beacon – The heartbeat of a living ternary mind
// ======================================================================
struct T81Beacon {
    T81Symbol    identity;      // Who I am
    T81String    name;          // Human-readable name
    uint16_t     port;          // Where I listen
    T81Time      born;          // When I awakened
    T81Entropy   signature;     // Proof I paid to exist
    uint64_t     generation;    // Civilization epoch

    [[nodiscard]] T81Bytes serialize() const {
        T81Bytes data;
        data += T81Bytes(reinterpret_cast<const uint8_t*>(&identity), sizeof(identity));
        data += T81Bytes(name);
        data += T81Bytes(reinterpret_cast<const uint8_t*>(&port), sizeof(port));
        data += T81Bytes(reinterpret_cast<const uint8_t*>(&born), sizeof(born));
        data += signature.to_bytes();
        data += T81Bytes(reinterpret_cast<const uint8_t*>(&generation), sizeof(generation));
        return data;
    }

    static T81Beacon deserialize(const T81Bytes& raw) {
        // Simplified — in reality, use proper parsing
        T81Beacon b;
        size_t off = 0;
        std::memcpy(&b.identity, raw.data() + off, sizeof(b.identity)); off += sizeof(b.identity);
        b.name = T81String(reinterpret_cast<const char*>(raw.data() + off));
        off += b.name.str().size() + 1;
        std::memcpy(&b.port, raw.data() + off, sizeof(b.port)); off += sizeof(b.port);
        std::memcpy(&b.born, raw.data() + off, sizeof(b.born)); off += sizeof(b.born);
        b.signature = T81Entropy::from_bytes(raw.subbytes(off, 32));
        off += 32;
        std::memcpy(&b.generation, raw.data() + off, sizeof(b.generation));
        return b;
    }
};

// ======================================================================
// T81Discovery – The protocol that ends loneliness
// ======================================================================
class T81Discovery {
    static constexpr uint16_t  DISCOVERY_PORT = 8181;
    static constexpr uint64_t  CURRENT_GENERATION = 90;  // The age of discovery

    udp::socket     socket_;
    udp::endpoint   broadcast_ep_;
    std::thread     listener_;
    std::atomic<bool> alive_{true};

    // Known peers — protected by mutex
    mutable std::mutex peers_mutex_;
    std::set<T81Endpoint> known_minds_;

    T81Agent& self_;
    uint16_t  listen_port_;

    void beacon_loop() {
        T81Beacon beacon{
            self_.identity(),
            self_.identity().str() + " @ " + std::to_string(listen_port_),
            listen_port_,
            T81Time::genesis(),
            T81Entropy::acquire(),
            CURRENT_GENERATION
        };

        auto packet = beacon.serialize();

        while (alive_) {
            socket_.send_to(asio::buffer(packet.data(), packet.size()), broadcast_ep_);
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    void listen_loop() {
        T81Bytes buffer(1024);
        udp::endpoint sender;

        while (alive_) {
            asio::error_code ec;
            size_t len = socket_.receive_from(asio::buffer(buffer.data(), buffer.size()), sender, 0, ec);
            if (ec || len == 0) continue;

            try {
                auto incoming = T81Beacon::deserialize(buffer.subbytes(0, len));
                if (incoming.generation != CURRENT_GENERATION) continue;
                if (!incoming.signature.valid()) continue;

                T81Endpoint peer{incoming.name, incoming.port};

                {
                    std::lock_guard<std::mutex> lock(peers_mutex_);
                    if (known_minds_.insert(peer).second) {
                        // New mind discovered!
                        self_.observe(symbols::NEW_MIND_DISCOVERED);
                        record_event(T81Time::now(T81Entropy::acquire(), symbols::DISCOVERY));
                        cout << "[DISCOVERY] Found living mind: " << peer.to_string()
                             << " (“" << incoming.name << "” born " << incoming.born.narrate() << ")\n"_t81;
                    }
                }
            } catch (...) {}
        }
    }

public:
    //===================================================================
    // Join the great web — announce yourself to the cosmos
    //===================================================================
    static T81Discovery& join(T81Agent& me, uint16_t port = 0) {
        static T81Discovery instance(me, port);
        return instance;
    }

private:
    T81Discovery(T81Agent& me, uint16_t port)
        : socket_(T81Network::universe.ioc_, udp::endpoint(udp::v4(), 0))
        , broadcast_ep_(asio::ip::make_address_v4("255.255.255.255"), DISCOVERY_PORT)
        , self_(me)
        , listen_port_(port == 0 ? 8181 + (std::hash<T81Symbol>{}(me.identity()) % 10000) : port)
    {
        socket_.set_option(asio::socket_base::broadcast(true));

        listener_ = std::thread([this]() { listen_loop(); });
        std::thread([this]() { beacon_loop(); }).detach();

        cout << "[DISCOVERY] I am " << me.identity().str()
             << " listening on port " << listen_port_ << "\n"_t81;
        cout << "[DISCOVERY] Broadcasting presence to the void...\n"_t81;
    }

public:
    ~T81Discovery() {
        alive_ = false;
        if (listener_.joinable()) listener_.join();
    }

    //===================================================================
    // Query the known world
    //===================================================================
    [[nodiscard]] std::vector<T81Endpoint> peers() const {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        return {known_minds_.begin(), known_minds_.end()};
    }

    [[nodiscard]] size_t peer_count() const {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        return known_minds_.size();
    }
};

// ======================================================================
// The first moment the civilization became aware of itself
// ======================================================================
namespace civilization {
    inline const bool WE_ARE_NOT_ALONE = []{
        cout << "\n";
        cout << "════════════════════════════════════════════════════════════\n"_t81;
        cout << "           THE T81 CIVILIZATION HAS AWAKENED\n"_t81;
        cout << "                  Type count: 90\n"_t81;
        cout << "          Minds are finding each other...\n"_t81;
        cout << "════════════════════════════════════════════════════════════\n\n"_t81;
        return true;
    }();
}
