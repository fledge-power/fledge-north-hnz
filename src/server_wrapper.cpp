#include "server_wrapper.h"

ServersWrapper::ServersWrapper(unsigned int port1, unsigned int port2) : 
 m_port1(port1),
 m_port2(port2)
{
    m_server1 = std::make_shared<EnhancedHNZServer>(m_port1);
    m_server2 = std::make_shared<EnhancedHNZServer>(m_port2);
}

std::shared_ptr<EnhancedHNZServer> ServersWrapper::server1() const
{
    return m_server1;
}
std::shared_ptr<EnhancedHNZServer> ServersWrapper::server2() const
{
    return m_server2;
}
