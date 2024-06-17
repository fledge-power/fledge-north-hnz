#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include "hnzutility.h"
#include "enhanced_hnz_server.h"

void EnhancedHNZServer::startHNZServer()
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::startHNZServer -";
  HnzUtility::log_info("%s [%d] server starting...", beforeLog.c_str(), m_port);
  if (server == nullptr)
  {
    server = new HNZServer();
  }
  std::lock_guard<std::mutex> guard(m_t1_mutex);
  m_t1 = new std::thread(&EnhancedHNZServer::m_start, server, m_port);
}

bool EnhancedHNZServer::joinStartThread()
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::joinStartThread -";

  // Lock to prevent multiple joins in parallel
  std::lock_guard<std::mutex> guard(m_t1_mutex);
  if (m_t1 != nullptr)
  {
    // m_t1 may never join because it is stuck on an IO operation...
    // The code below ensure that we can attempt to join and exit this function regardless in a finished time
    std::atomic<bool> joinSuccess{false};
    std::thread joinThread([this, &joinSuccess]()
                           {
      m_t1->join();
      joinSuccess = true; });
    joinThread.detach();
    int timeMs = 0;
    // Wait up to 10s for m_t1 thread to join
    while (timeMs < 10000)
    {
      if (joinSuccess)
      {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      timeMs += 100;
    }
    if (!joinSuccess)
    {
      HnzUtility::log_info("%s [%d] Could not join m_t1, exiting", beforeLog.c_str(), m_port);
      // Do not delete m_t1 if it was not joined or it will crash immediately, let the rest of the test complete first
      return false;
    }
    delete m_t1;
    m_t1 = nullptr;
  }
  return true;
}

void EnhancedHNZServer::stopHNZServer()
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::stopHNZServer -";
  HnzUtility::log_info("%s [%d]  Server stopping...", beforeLog.c_str(), m_port);
  is_running = false;
  m_state_machine->reset();

  // Ensure that m_t1 was joined no matter what
  joinStartThread();

  // Stop HNZ server before joining receiving_thread
  if (server != nullptr)
  {
    server->stop();
    server = nullptr;
  }

  // Lock to wait for EnhancedHNZServerIsReady to complete if it was running
  std::lock_guard<std::mutex> guard(m_init_mutex);
  if (receiving_thread != nullptr)
  {
    receiving_thread->join();
    delete receiving_thread;
    receiving_thread = nullptr;
  }

  // Free HNZ server after receiving_thread was joined as it uses that object
  if (server != nullptr)
  {
    delete server;
    server = nullptr;
  }
  HnzUtility::log_info("%s [%d]  server stopped!", beforeLog.c_str(), m_port);
}

size_t EnhancedHNZServer::checkReceivingControl(unsigned char control, int buffer_length)
{

  size_t len = 0;
  switch (control)
  {
  // UA
  case UA_CODE:
    len = 0;
    sendSARM();
    m_state_machine->receiveSARMCode();
    break;
  // SARM
  case SARM_CODE:
    len = 0;
    sendUA();
    m_state_machine->receiveUACode();
    break;
  // DISC
  case 0x43:
    len = 0;
    sendUA();
    break;
  default:
    len = buffer_length;
    break;
  }

  return len;
}

void EnhancedHNZServer::checkInformationFrame(size_t len,unsigned char * data,int p )
{

  if(len < 3) {
    return;
  }

  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::checkInformationFrame -";
    // CG
    if (len > 3 && data[2] == CG_CODE && data[3] == 0x1)
    {
      HnzUtility::log_debug("%s [%d]  Receive CG", beforeLog.c_str(), m_port);
    }
    // Message bulle
    else if (len > 3 && data[2] == CG_CODE && data[3] == 0x4)
    {
      HnzUtility::log_debug("%s [%d]  Receive bulle message", beforeLog.c_str(), m_port);
      sendRR(p);
    }
    // TC message
    else if (len > 3 && data[2] == TC_CODE )
    {
      sendRR(p);
      const auto ado = data[3];
      const auto adb = (data[4] & 0b11100000) >> 5;
      const auto open_order = ((data[4] & 0b11000) >> 3) == 0b10;
      HnzUtility::log_debug("%s [%d]  TC message ADO=%d ADB=%d order_open=%s", beforeLog.c_str(), m_port, ado, adb, open_order ? "1" : "0");
      std::vector<unsigned char> message(3, 0);
      message[0] = TCACK_CODE;
      message[1] = ado;
      message[2] = 0b1;
      message[2] |= adb << 5;
      message[2] |= (open_order ? 0b10 : 0b1) << 3;

      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
      sendInformation(message, p);
    }
    // update time
    else if (data[2] == 0x1D)
    {
      HnzUtility::log_debug("%s [%d]  Update time", beforeLog.c_str(), m_port);
      sendRR(p);
    }
    // update date
    else if (data[2] == 0x1C && len > 5)
    {
      HnzUtility::log_debug("%s [%d]  Update date %d/%d/%d", beforeLog.c_str(), m_port, data[3], data[4] + 1, 1930 + data[5]);
      sendRR(p);
    }
    else if (data[2] == TVC_CODE)
    {
      const auto addr = data[3];
      const auto open_order = (data[4] >> 3) == 0b10;
      HnzUtility::log_debug("%s [%d]  TVC message ADDR=%d  order_open=%s", beforeLog.c_str(), m_port, addr, open_order ? "1" : "0");
      sendRR(p);
    }
    else
    {
      HnzUtility::log_warn("%s [%d]  UNKNOWN information command", beforeLog.c_str(), m_port);
      sendFrame({(unsigned char)m_state_machine->getControlREJ(p)}, false);
    }
}

void EnhancedHNZServer::analyzeReceivingFrame(MSG_TRAME *const frReceived)
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::analyzeReceivingFrame -";

  if (frReceived == nullptr || frReceived->usLgBuffer == 0)
  {
    return;
  }

  HnzUtility::log_debug("%s [%d] %s", beforeLog.c_str(), m_port, EnhancedHNZServer::frameToStr(frReceived).c_str());
  const auto data = frReceived->aubTrame;
  const auto control = data[1];

  const auto len = checkReceivingControl(control, frReceived->usLgBuffer);

  auto p = (control & 0b00010000) >> 4;
  auto information = control & 0b00000001;

  if (information == 0 && len > 2)
  {
    HnzUtility::log_debug("%s [%d] information frame", beforeLog.c_str(), m_port);
    m_state_machine->receiveInformation();
    checkInformationFrame(len,data,p);
  }
  else
  {
    auto rr = (control & 0b00001111);
    if (rr == 1)
    {
      HnzUtility::log_debug("%s [%d]  RR command", beforeLog.c_str(), m_port);
    }
    else if (rr == 0b1001)
    {
      HnzUtility::log_debug("%s [%d]  REJ command", beforeLog.c_str(), m_port);
    }
    else
    {
      HnzUtility::log_warn("%s [%d]  UNKNOWN command", beforeLog.c_str(), m_port);
    }
  }
}

void EnhancedHNZServer::receiving_loop()
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::receiving_loop -";
  while (is_running)
  {
    // Receive an hnz frame, this call is blocking
    const auto frReceived = server->receiveFr();

    if (frReceived != nullptr)
    {
      if (!server->checkCRC(frReceived))
      {
        HnzUtility::log_warn("%s [%d]  The CRC does not match", beforeLog.c_str(), m_port);
        continue;
      }
      analyzeReceivingFrame(frReceived);
      // Store the frame that was received for testing purposes
      auto newFrame = std::make_shared<MSG_TRAME>();
      memcpy(newFrame.get(), frReceived, sizeof(MSG_TRAME));
      onFrameReceived(newFrame);
    }
    if (!server->isConnected())
    {
      HnzUtility::log_info("%s [%d]  TCP Connection lost, exit receiving_loop", beforeLog.c_str(), m_port);
      fflush(stdout);
      is_running = false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  HnzUtility::log_info("%s [%d] receiving_loop terminated", beforeLog.c_str(), m_port);
}

void EnhancedHNZServer::sendSARMLoop()
{
  // Reset SARM/UA variables in case of reconnect
  {
  std::lock_guard<std::mutex> guard(m_sarm_ua_mutex);
  ua_ok = false;
  sarm_ok = false;
  }

  bool sarm_ua_ok = false;
  while (is_running && !sarm_ua_ok)
  {
    if (server->isConnected())
    {
      sendSARM();
      std::lock_guard<std::mutex> guard(m_sarm_ua_mutex);
      ua_ok = false;
      sarm_ok = false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    std::lock_guard<std::mutex> guard(m_sarm_ua_mutex);
    sarm_ua_ok = ua_ok && sarm_ok;
  }
}

bool EnhancedHNZServer::waitForTCPConnection(int timeout_s)
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::waitForTCPConnection -";
  const auto start = time(nullptr);
  while (!server->isConnected() && is_running)
  {
    if (time(nullptr) - start > timeout_s)
    {
      HnzUtility::log_info("%s [%d] Connection timeout", beforeLog.c_str(), m_port);
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  return true;
}

void EnhancedHNZServer::handChecking(int timeout_s ) {

  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::handChecking -";

    // Wait for UA and send UA in response of SARM
  auto start = time(nullptr);
  bool lastFrameWasEmpty = false;

  // Make sure to always exit this loop with is_running==false, not return
  // to ensure that m_t2 is joined properly
  while (is_running)
  {
    if (time(nullptr) - start > timeout_s)
    {
      HnzUtility::log_info("%s [%d] SARM/UA timeout", beforeLog.c_str(), m_port);
      is_running = false;
      break;
    }
    if (!server->isConnected())
    {
      HnzUtility::log_info("%s [%d] Connection lost, restarting server...", beforeLog.c_str(), m_port);
      // Server is still not connected? restart the server
      server->stop();
      startHNZServer();
      if (!waitForTCPConnection(timeout_s))
      {
        is_running = false;
        break;
      }
      if (!is_running)
      {
        HnzUtility::log_info("%s [%d] Not running after reconnection, exit", beforeLog.c_str(), m_port);
        break;
      }
      start = time(nullptr);
      HnzUtility::log_info("%s [%d] Server reconnected!", beforeLog.c_str(), m_port);
    }

    auto frReceived = (server->receiveFr());
    if (frReceived == nullptr)
    {
      if (!lastFrameWasEmpty)
      {
        lastFrameWasEmpty = true;
        HnzUtility::log_info("%s [%d] Received empty frame", beforeLog.c_str(), m_port);
      }
      continue;
    }
    lastFrameWasEmpty = false;
    const auto data = frReceived->aubTrame;
    const auto c = data[1];
    switch (c)
    {
      case UA_CODE:
      {
        HnzUtility::log_info("%s [%d] UA received", beforeLog.c_str(), m_port);
        m_state_machine->receiveUACode();
        std::lock_guard<std::mutex> guard2(m_sarm_ua_mutex);
        ua_ok = true;
        break;
      }
      case SARM_CODE:
      {
        HnzUtility::log_info("%s [%d] SARM received, sending UA", beforeLog.c_str(), m_port);
        m_state_machine->receiveSARMCode();
        sendUA();
        std::lock_guard<std::mutex> guard2(m_sarm_ua_mutex);
        sarm_ok = true;
        break;
      }
      default:
      {
        HnzUtility::log_info("%s [%d] either UA nor SARM: %d", beforeLog.c_str(), m_port, static_cast<int>(c));
        break;
      }
    }
    // Store the frame that was received for testing purposes
    auto newFrame = std::make_shared<MSG_TRAME>();
    memcpy(newFrame.get(), frReceived, sizeof(MSG_TRAME));
    onFrameReceived(newFrame);
    std::lock_guard<std::mutex> guard2(m_sarm_ua_mutex);
    if (ua_ok && sarm_ok)
    {
      // clear frame
      popLastFramesReceived();
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

}
bool EnhancedHNZServer::ServerIsReady(int timeout_s /*= 16*/)
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::ServerIsReady -";
  // Lock to prevent multiple calls to this function in parallel
  std::lock_guard<std::mutex> guard(m_init_mutex);
  if (receiving_thread)
  {
    return true;
  }
  is_running = true;

  HnzUtility::log_info("%s [%d] Waiting for initial connection...", beforeLog.c_str(), m_port);

  // Wait for the server to finish starting
  if (!waitForTCPConnection(timeout_s))
  {
    return false;
  }
  if (!is_running)
  {
    HnzUtility::log_info("%s [%d] Not running after initial connection, exit", beforeLog.c_str(), m_port);
    return false;
  }

  // Loop that sending sarm every 3s
  std::thread thread_sarmloop (&EnhancedHNZServer::sendSARMLoop, this);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  handChecking(timeout_s);

  thread_sarmloop.join();

  if (!is_running)
  {
    HnzUtility::log_info("%s [%d] Not running after SARM/UA, exit", beforeLog.c_str(), m_port);
    return false;
  }
  HnzUtility::log_info("%s [%d] Connection OK !!", beforeLog.c_str(), m_port);

  // Connection established
  receiving_thread = new std::thread(&EnhancedHNZServer::receiving_loop, this);
  return true;
}

void EnhancedHNZServer::sendSARM()
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::sendSARM -";
  HnzUtility::log_info("%s [%d] Send SARM !!", beforeLog.c_str(), m_port);
  uint8_t message[1];
  message[0] = SARM_CODE;

  createAndSendFrame(static_cast<unsigned char>(m_state_machine->getBAddress()), message, sizeof(message));
}

void EnhancedHNZServer::sendRR(bool repetition)
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::sendRR -";
  HnzUtility::log_info("%s [%d] Sending RR...", beforeLog.c_str(), m_port);
  std::vector<unsigned char> message;
  message.push_back(static_cast<unsigned char>(m_state_machine->getControlRR(repetition ? 1 : 0)));
  createAndSendFrame(static_cast<unsigned char>(m_state_machine->getAAddress()), message.data(), static_cast<int>(message.size()));
}

void EnhancedHNZServer::sendUA()
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::sendUA -";
  HnzUtility::log_info("%s [%d] Sending UA...", beforeLog.c_str(), m_port);
  std::vector<unsigned char> message;
  message.push_back(UA_CODE);
  createAndSendFrame(static_cast<unsigned char>(m_state_machine->getAAddress()), message.data(), static_cast<int>(message.size()));
}

void EnhancedHNZServer::sendInformation(const std::vector<unsigned char> &message, bool repeat)
{
  sendFrame(message, repeat);
  m_state_machine->sendInformation();
}

void EnhancedHNZServer::sendFrame(std::vector<unsigned char> message, bool repeat)
{
  auto num = static_cast<unsigned char>(m_state_machine->getControlInformation(repeat));
  message.insert(message.begin(), num);
  createAndSendFrame(static_cast<unsigned char>(m_state_machine->getBAddress()), message.data(), static_cast<int>(message.size()));
}

void EnhancedHNZServer::createAndSendFrame(unsigned char addr, const unsigned char *msg, int msgSize)
{
  std::string beforeLog = HnzUtility::PluginName + " - EnhancedHNZServer::createAndSendFrame -";
  if (server != nullptr)
  {
    // Code extracted from HNZClient::createAndSendFr of libhnz
    auto pTrame = std::make_shared<MSG_TRAME>();
    uint8_t msgWithAddr[msgSize + 1];
    // Add address
    msgWithAddr[0] = addr;
    // Add message
    memcpy(msgWithAddr + 1, msg, msgSize);
    server->addMsgToFr(pTrame.get(), msgWithAddr, static_cast<int>(sizeof(msgWithAddr)));
    server->setCRC(pTrame.get());
    // Store the frame about to be sent for testing purposes
    onFrameSent(pTrame);
    HnzUtility::log_debug("%s [%d] %s", beforeLog.c_str(), m_port, frameToStr(pTrame).c_str());
    server->sendFr(pTrame.get());
  }
}

std::vector<std::shared_ptr<MSG_TRAME>> EnhancedHNZServer::popLastFramesReceived()
{
  std::lock_guard<std::mutex> guard(m_received_mutex);
  std::vector<std::shared_ptr<MSG_TRAME>> tmp = m_last_frames_received;
  m_last_frames_received.clear();
  return tmp;
}

std::vector<std::shared_ptr<MSG_TRAME>> EnhancedHNZServer::popLastFramesSent()
{
  std::lock_guard<std::mutex> guard(m_sent_mutex);
  std::vector<std::shared_ptr<MSG_TRAME>> tmp = m_last_frames_sent;
  m_last_frames_sent.clear();
  return tmp;
}

void EnhancedHNZServer::onFrameReceived(const std::shared_ptr<MSG_TRAME> &frame)
{
  std::lock_guard<std::mutex> guard(m_received_mutex);
  auto newFrame = std::make_shared<MSG_TRAME>();
  memcpy(newFrame.get(), frame.get(), sizeof(MSG_TRAME));
  m_last_frames_received.push_back(newFrame);
}

void EnhancedHNZServer::onFrameSent(const std::shared_ptr<MSG_TRAME> &frame)
{
  std::lock_guard<std::mutex> guard(m_sent_mutex);
  auto newFrame = std::make_shared<MSG_TRAME>();
  memcpy(newFrame.get(), frame.get(), sizeof(MSG_TRAME));
  m_last_frames_sent.push_back(newFrame);
}

std::string EnhancedHNZServer::toHexStr(unsigned char num)
{
  std::stringstream stream;
  stream << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(num);
  return stream.str();
}

std::string EnhancedHNZServer::frameToStr(const std::shared_ptr<MSG_TRAME> &frame)
{
  return frameToStr(frame.get());
}

std::string EnhancedHNZServer::frameToStr(const MSG_TRAME *frame)
{
  if(frame == nullptr) 
  {
    return {};
  }

  std::stringstream stream;
  stream << "\n[";
  for (int i = 0; i < frame->usLgBuffer; i++)
  {
    if (i > 0)
    {
      stream << ", ";
    }
    stream << toHexStr(frame->aubTrame[i]);
  }
  stream << "]";
  return stream.str();
}

std::string EnhancedHNZServer::framesToStr(const std::vector<std::shared_ptr<MSG_TRAME>> &frames)
{
  std::stringstream stream;
  for (auto frame : frames)
  {
    stream << frameToStr(frame);
  }
  return stream.str();
}