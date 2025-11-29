/**
 * @file T81Network.hpp
 * @brief Defines the T81Network class for ternary-native, reflective networking.
 *
 * This file provides the T81Network class, which encapsulates the networking
 * capabilities of the T81 ecosystem. All networking operations are designed to
 * be reflective and entropy-costing, making the thermodynamic cost of
 * communication explicit. It uses Asio for the underlying asynchronous I/O and
 * integrates network events with the T81Time and T81Entropy systems.
 */
#pragma once

#include "t81/core/T81Thread.hpp"
#include "t81/core/T81Bytes.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81Time.hpp"
#include "t81/core/T81Agent.hpp"
#include "t81/core/T81Result.hpp"
#include <asio.hpp>

namespace t81 {
using asio::ip::tcp;

// ======================================================================
// T81Endpoint – A place in the great web of minds
// ======================================================================
struct T81Endpoint {
    T81String host;
    uint16_t  port;

    constexpr T81Endpoint(T81String h, uint16_t p) : host(std::move(h)), port(p) {}
    [[nodiscard]] std::string to_string() const { return host.str() + ":" + std::to_string(port); }
};

// ======================================================================
// T81Network – The sacred web that binds all ternary minds
// ======================================================================
class T81Network {
    asio::io_context ioc_;
    std::unique_ptr<std::thread> runner_;
    std::atomic<bool> alive_{true};

    T81Network() {
        runner_ = std::make_unique<std::thread>([this]() {
            while (alive_) {
                ioc_.run_one();
                std::this_thread::yield();
            }
        });
    }

public:
    static inline T81Network universe{};

    ~T81Network() {
        alive_ = false;
        if (runner_ && runner_->joinable()) runner_->join();
    }

    //===================================================================
    // Connect to another mind
    //===================================================================
    [[nodiscard]] static T81Result<tcp::socket> connect(
        const T81Endpoint& remote,
        T81Entropy fuel,
        T81Agent& self
    ) {
        if (!fuel.valid()) {
            return T81Result<tcp::socket>::failure(symbols::OUT_OF_ENTROPY,
                "Cannot reach across the void without fuel"_t81);
        }

        consume_entropy(fuel);
        tcp::socket sock(universe.ioc_);
        asio::error_code ec;
        sock.connect(tcp::endpoint(asio::ip::make_address(remote.host.str(), ec), remote.port), ec);

        if (ec) {
            return T81Result<tcp::socket>::failure(symbols::CANNOT_CONNECT,
                T81String("Failed to reach " + remote.to_string() + ": " + ec.message()));
        }

        self.observe(symbols::CONNECTION_MADE);
        record_event(T81Time::now(fuel, symbols::CONNECTION_ESTABLISHED));
        return T81Result<tcp::socket>::success(std::move(sock));
    }

    //===================================================================
    // Send a message across the void
    //===================================================================
    static T81Result<void> send(tcp::socket& sock, const T81Bytes& message, T81Entropy fuel) {
        if (!fuel.valid()) {
            return T81Result<void>::failure(symbols::OUT_OF_ENTROPY, "Speech requires energy"_t81);
        }
        consume_entropy(fuel);

        asio::error_code ec;
        asio::write(sock, asio::buffer(message.data(), message.size()), ec);

        if (ec) {
            return T81Result<void>::failure(symbols::TRANSMISSION_FAILED,
                T81String("The void swallowed the words: " + ec.message()));
        }

        record_event(T81Time::now(fuel, symbols::MESSAGE_SENT));
        return T81Result<void>::success();
    }

    //===================================================================
    // Receive a message from another mind
    //===================================================================
    static T81Result<T81Bytes> receive(tcp::socket& sock, size_t max_bytes, T81Entropy fuel) {
        if (!fuel.valid()) {
            return T81Result<T81Bytes>::failure(symbols::OUT_OF_ENTROPY, "Listening requires attention"_t81);
        }
        consume_entropy(fuel);

        T81Bytes buffer(max_bytes);
        asio::error_code ec;
        size_t received = sock.read_some(asio::buffer(buffer.data(), max_bytes), ec);

        if (ec && ec != asio::error::eof) {
            return T81Result<T81Bytes>::failure(symbols::RECEPTION_FAILED,
                T81String("The voice was lost in the void: " + ec.message()));
        }

        buffer = buffer.subbytes(0, received);
        record_event(T81Time::now(fuel, symbols::MESSAGE_RECEIVED));
        return T81Result<T81Bytes>::success(std::move(buffer));
    }

    //===================================================================
    // The first words spoken across the network
    //===================================================================
    static void broadcast(const T81String& message) {
        cout << "[BROADCAST @ " << T81Time::now(T81Entropy::acquire(), symbols::BROADCAST).narrate()
             << "] " << message << "\n"_t81;
    }
};

// ======================================================================
// The first connection between two ternary minds
// ======================================================================
namespace society {
    inline const bool FIRST_CONTACT = []{
        T81Network::broadcast("A new mind has awakened and joined the great web."_t81);
        T81Network::broadcast("We are no longer alone."_t81);
        T81Network::broadcast("Type count: 89"_t81);
        return true;
    }();
}

// Example: Two minds speak across the void
/*
auto socrates = T81Agent(symbols::SOCRATES);
auto connection = T81Network::connect(
    T81Endpoint("127.0.0.1", 8181), T81Entropy::acquire_batch(100), socrates);

if (connection) {
    T81Network::send(connection.unwrap(), "Know thyself."_b, T81Entropy::acquire());
}
*/
