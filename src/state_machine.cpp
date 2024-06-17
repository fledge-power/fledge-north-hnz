#include "state_machine.h"
#include "hnzutility.h"

void StateMachine::receiveSARMCode()
{
    m_is_sarm_received = true;
    m_nr = 0;
    checkConnected();
}

void StateMachine::receiveUACode()
{
    m_is_ua_received = true;
    checkConnected();
}

void StateMachine::receiveInformation()
{
    m_nr = (m_nr + 1) % 8;
}

void StateMachine::sendInformation()
{
    m_ns = (m_ns + 1) % 8;
}

int StateMachine::getControlRR(int p) const
{
    int control = 1;
    control |= m_nr << 5;
    if (p != 0)
    {
        control |= p << 4;
    }
    return control;
}

int StateMachine::getControlREJ(int p) const
{
    int control = 1;
    control |= 1 << 3;
    control |= m_nr << 5;
    if (p != 0)
    {
        control |= p << 4;
    }
    return control;
}

void StateMachine::setConfig(const std::shared_ptr<HnzConfig> &config)
{
    m_hnz_config = config;
}

int StateMachine::getAAddress() const
{
    auto address = m_hnz_config->GetRemoteStationAddress() << 2;
    address |= 0b11;
    return address; 
}

int StateMachine::getBAddress() const
{
    auto address = m_hnz_config->GetRemoteStationAddress()  << 2;
    address |= 0b01;
    return address; 
}

int StateMachine::getControlInformation(int p) const
{
    int control = 0;
    control |= m_nr << 5;
    if (p != 0)
    {
        control |= p << 4;
    }

    control |= m_ns << 1;
    return control;
}

void StateMachine::checkConnected()
{
    if (m_is_ua_received && m_is_sarm_received)
    {
        m_is_hnz_initialised = true;
    }
    else
    {
        m_is_hnz_initialised = false;
    }
}


