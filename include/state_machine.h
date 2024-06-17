#pragma once
#include <memory>
#include "hnz_config.h"
#include "pa_hnz_data.h"
#include <mutex>
/**
 * @class StateMachine
 * @brief Manages the state and transitions for a communication protocol.
 *
 * The StateMachine class is responsible for handling various states and 
 * transitions within a communication protocol. It provides methods for 
 * receiving and sending information, managing control codes, and configuring 
 * the machine.
 */
class StateMachine {
public:

    /**
     * @brief Handles the reception of a SARM (Set Asynchronous Response Mode) code.
     *
     * This method processes the SARM code received in the communication protocol.
     */
    void receiveSARMCode();

    /**
     * @brief Handles the reception of a UA (Unnumbered Acknowledge) code.
     *
     * This method processes the UA code received in the communication protocol.
     */
    void receiveUACode();

    /**
     * @brief Handles the reception of information frames.
     *
     * This method processes information frames received in the communication protocol.
     */
    void receiveInformation();

    /**
     * @brief Sends information frames.
     *
     * This method is used to send information frames within the communication protocol.
     */
    void sendInformation();

    /**
     * @brief Generates a Receiver Ready (RR) control code.
     *
     * @param p The parameter to be used in generating the RR code.
     * @return The generated RR control code.
     *
     * This method generates and returns an RR control code based on the given parameter.
     */
    int getControlRR(int p) const;

    /**
     * @brief Generates a Reject (REJ) control code.
     *
     * @param p The parameter to be used in generating the REJ code.
     * @return The generated REJ control code.
     *
     * This method generates and returns a REJ control code based on the given parameter.
     */
    int getControlREJ(int p) const;

    /**
     * @brief Generates an Information control code.
     *
     * @param p The parameter to be used in generating the information control code.
     * @return The generated information control code.
     *
     * This method generates and returns an information control code based on the given parameter.
     */
    int getControlInformation(int p) const;

    /**
     * @brief Is Center connected
     *
     * This method verifies the connection status of the state machine.
     * @return Connected
     */
    bool isConnected() const { return m_is_ua_received && m_is_sarm_received;}

    /**
     * @brief Sets the configuration for the state machine.
     *
     * @param config A shared pointer to an HnzConfig object containing the configuration settings.
     *
     * This method sets up the state machine configuration using the provided HnzConfig object.
     */
    void setConfig(const std::shared_ptr<HnzConfig>& config);

    /**
     * @brief Retrieves the A address.
     *
     * @return The A address.
     *
     * This method returns the A address used in the communication protocol.
     */
    int getAAddress() const;

    /**
     * @brief Retrieves the B address.
     *
     * @return The B address.
     *
     * This method returns the B address used in the communication protocol.
     */
    int getBAddress() const;

    /**
     * @brief Retrieves pa hnz data
     *
     * @return pa hnz data.
     *
     * This method returns pa hnz data.
     */
    PaHnzData & getHnzData()   {
        return m_hnz_data;
    }

    /**
     * @brief Retrieves CG sent
     *
     * @return is cg sent
     *
     * This method returns cg sent.
     */
    bool isCGSent() const {
        return m_cg_sent;
    }

    /**
     * @brief set CG sent
     *
     *
     * This method set cg sent.
     */
    void setCgSent(bool cg) {
        m_cg_sent = cg;
    }

    /**
     * @brief reset
     *
     *
     * reset state machine
     */
    void reset() {
        m_is_hnz_initialised = false;
        m_is_sarm_received = false;
        m_is_ua_received = false;
        m_nr = 0;
        m_ns = 0;
        m_cg_sent = false;
    }
private:

    /**
     * @brief Checks if the state machine is connected.
     *
     * This method verifies the connection status of the state machine.
     */
    void checkConnected();

    bool m_is_hnz_initialised{false};
    bool m_is_sarm_received{false};
    bool m_is_ua_received{false};
    int m_nr{0};
    int m_ns{0};
    bool m_cg_sent{false};
    std::shared_ptr<HnzConfig> m_hnz_config;
    PaHnzData m_hnz_data;


};