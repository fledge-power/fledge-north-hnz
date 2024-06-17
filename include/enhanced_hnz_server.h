#pragma once

#include <chrono>
#include <thread>
#include <mutex>
#include "state_machine.h"
#include "hnz_server.h"

const unsigned char CG_CODE = 0x13;
/**
 * Represents an enhanced HNZ server.
 */
class EnhancedHNZServer
{
public:
  /**
   * Constructor to initialize the server with a port and an address.
   * @param port The port number for the server.
   */
  explicit EnhancedHNZServer(int port) : m_port(port)
  {
    m_state_machine = std::make_shared<StateMachine>();
  };

  EnhancedHNZServer(EnhancedHNZServer const &) = delete;
  EnhancedHNZServer &operator=(EnhancedHNZServer const &) = delete;

  /**
   * Starts the HNZ server.
   */
  void startHNZServer();

  /**
   * Stops the HNZ server.
   */
  void stopHNZServer();

  /**
   * Joins the start thread.
   * @return True if the thread is successfully joined, false otherwise.
   */
  bool joinStartThread();

  /**
   * Waits for a TCP connection.
   * @param timeout_s Timeout duration in seconds.
   * @return True if a TCP connection is established within the timeout, false otherwise.
   */
  bool waitForTCPConnection(int timeout_s);

  /**
   * Checks if the server is ready.
   * @param timeout_s Timeout duration in seconds.
   * @return True if the server is ready within the timeout, false otherwise.
   */
  bool ServerIsReady(int timeout_s = 16);

  /**
   * Sends SARM code.
   */
  void sendSARM();

  /**
   * Sends UA code.
   */
  void sendUA();

  /**
   * Sends RR code.
   */
  void sendRR(bool repetition);

  /**
   * @brief Analyzes the received frame.
   *
   * This function takes a pointer to a `MSG_TRAME` structure representing the received frame
   * and performs the necessary analysis on it.
   *
   * @param frReceived A pointer to the `MSG_TRAME` structure that contains the received frame data.
   *
   * This function does not return a value. It performs its analysis in-place or may modify
   * other states or outputs based on the analysis.
   */
  void analyzeReceivingFrame(MSG_TRAME * const frReceived);

  /**
   * @brief Checks and processes the information frame.
   * 
   * This method checks the information frame provided by the data pointer with a specified length 
   * and an additional parameter for processing.
   * 
   * @param len The length of the data.
   * @param data A pointer to an array of unsigned char representing the data of the information frame.
   * @param p An integer parameter used for repetition frame
   * 
  */
  void checkInformationFrame(size_t len,unsigned char * data,int p);

  /**
   * @brief Checks the receiving control byte and returns a status or length.
   *
   * This function processes the control byte of a received message and performs
   * necessary checks based on the provided buffer length. The exact purpose of the
   * function (e.g., validating control byte, determining message length) should be
   * clarified based on the implementation details.
   *
   * @param control The control byte of the received message.
   * @param buffer_length The length of the buffer containing the message.
   * 
   * @return A `size_t` value indicating the result of the check. The meaning of the
   *         returned value (e.g., status code, validated length) should be specified
   *         based on the implementation.
   *
   * @note Ensure that `buffer_length` is non-negative. The function behavior is
   *       undefined if `buffer_length` is negative.
   *
   * Example usage:
   * @code
   * unsigned char control = 0x1A;  Example control byte
   * int buffer_length = 1024;      Example buffer length
   * size_t result = checkReceivingControl(control, buffer_length);
   * Use result as needed
   * @endcode
   **/
  size_t checkReceivingControl(unsigned char control,  int buffer_length);

  /**
   * Sends information frame with the specified message.
   * @param message The message to be sent.
   * @param repeat Whether to repeat the frame.
   */
  void sendInformation(const std::vector<unsigned char>& message, bool repeat);

  /**
   * Sends a frame with the specified message.
   * @param message The message to be sent.
   * @param repeat Whether to repeat the frame.
   */
  void sendFrame(std::vector<unsigned char> message, bool repeat);

  /**
   * Creates and sends a frame with the given address and message.
   * @param addr The address of the frame.
   * @param msg The message to be sent.
   * @param msgSize The size of the message.
   */
  void createAndSendFrame(unsigned char addr, const unsigned char *msg, int msgSize);

  /**
   * Retrieves and removes the last received frames.
   * @return A vector containing the last received frames.
   */
  std::vector<std::shared_ptr<MSG_TRAME>> popLastFramesReceived();

  /**
   * Retrieves and removes the last sent frames.
   * @return A vector containing the last sent frames.
   */
  std::vector<std::shared_ptr<MSG_TRAME>> popLastFramesSent();

  /**
   * Handles the received frame.
   * @param frame The received frame.
   */
  void onFrameReceived(const std::shared_ptr<MSG_TRAME>&frame);

  /**
   * Handles the sent frame.
   * @param frame The sent frame.
   */
  void onFrameSent(const std::shared_ptr<MSG_TRAME>&frame);

  /**
   * Disables or enables acknowledgments.
   * @param disabled True to disable acknowledgments, false to enable.
   */
  void disableAcks(bool disabled) { ack_disabled = disabled; }

  /**
   * Converts an unsigned char to its hexadecimal string representation.
   * @param num The unsigned char to convert.
   * @return The hexadecimal string representation of the number.
   */
  static std::string toHexStr(unsigned char num);

  /**
   * Converts a frame to its string representation.
   * @param frame The frame to convert.
   * @return The string representation of the frame.
   */
  static std::string frameToStr(const std::shared_ptr<MSG_TRAME> &frame);

  /**
   * Converts a frame to its string representation.
   * @param frame The frame to convert.
   * @return The string representation of the frame.
   */
  static std::string frameToStr(const MSG_TRAME* frame);

  /**
   * Converts a vector of frames to their string representation.
   * @param frames The vector of frames to convert.
   * @return The string representation of the frames.
   */
  static std::string framesToStr(const std::vector<std::shared_ptr<MSG_TRAME>>& frames);

    /**
   * Gets tcp port
   * @return The tcp port
   */
  int getPort() const { return m_port;}

    /**
   * Gets the state machine
   * @return The state machine
   */
  std::shared_ptr<StateMachine> getStateMachine() const { return m_state_machine; }

  /**
   * The main loop for receiving frames.
   */
  void receiving_loop();

  /**
   * The loop for sending SARM signals.
   */
  void sendSARMLoop();

  /**
   * Starts the server on a specified port.
   * @param server The server instance.
   * @param port The port to start the server on.
   */
  static void m_start(HNZServer *server, int port) { server->start(port); };

  /**
   * Hand shaking between PA and center. Send SARM and UA code.
   * @param timeout_s Timeout duration in seconds
   */
  void handChecking(int timeout_s );

  /**
   * Set is server running
   * @param running The server running
   */
  void setIsRunning(bool running) { is_running = running;}

  /**
   * Gets is server running
   * @return The server running
   */
  bool isRunning() const {return is_running;} 

private:

  HNZServer *server = nullptr;


  std::thread *m_t1 = nullptr;
  std::mutex m_t1_mutex;

  std::thread *receiving_thread = nullptr;
  std::mutex m_init_mutex;

  std::atomic<bool> is_running{false};
  std::atomic<bool> ack_disabled{false};

  int m_port = 0;

  bool ua_ok = false;
  bool sarm_ok = false;
  std::mutex m_sarm_ua_mutex;

  std::vector<std::shared_ptr<MSG_TRAME>> m_last_frames_received;
  std::mutex m_received_mutex;

  std::vector<std::shared_ptr<MSG_TRAME>> m_last_frames_sent;
  std::mutex m_sent_mutex;
  std::shared_ptr<StateMachine> m_state_machine;
};