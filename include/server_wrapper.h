
#pragma once 

#include <memory>
#include "enhanced_hnz_server.h"

/**
 * @brief A class to manage two instances of EnhancedHNZServer.
 */
class ServersWrapper {
public:
    /**
     * @brief Constructor for ServersWrapper.
     * @param port1 The port number for server1.
     * @param port2 The port number for server2.
     */
    ServersWrapper(unsigned int port1, unsigned int port2);

    /**
     * @brief Get the shared pointer to server1.
     * @return A shared pointer to server1.
     */
    std::shared_ptr<EnhancedHNZServer> server1() const;

    /**
     * @brief Get the shared pointer to server2.
     * @return A shared pointer to server2.
     */
    std::shared_ptr<EnhancedHNZServer> server2() const;

private:
    std::shared_ptr<EnhancedHNZServer> m_server1; /**< Shared pointer to server1. */
    std::shared_ptr<EnhancedHNZServer> m_server2; /**< Shared pointer to server2. */
    unsigned int m_addr{0};
    unsigned int m_port1{0}; /**< Port number for server1. */
    unsigned int m_port2{0}; /**< Port number for server2. */
};