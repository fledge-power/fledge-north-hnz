#include "hnz.h"
#include "hnzutility.h"
#include <reading.h>
#include <string_utils.h>
#include <memory>
#include <config_category.h>
#include <rapidjson/document.h>
#include "server_wrapper.h"
#include <iostream>
#include <algorithm>

// The duration, in milliseconds, for which the thread server sleeps before checking for io.
static const int THREAD_SERVER_SLEEPING_DURATION_IN_MS = 2000;

// The duration, in seconds, for which the thread is server ready wait.
static const int SERVER_IS_READY_DURATION = 10;

// The duration, in milliseconds, for which the pivot object is sent to the center
static const int PIVOT_TO_HNZ_CENTER_DURATION = 500;

void HNZ::start()
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::start -";

  if (m_is_running)
  {
    HnzUtility::log_info("%s HNZ north plugin already started", beforeLog.c_str());
    return;
  }

  HnzUtility::log_info("%s Starting HNZ north plugin...", beforeLog.c_str());

  m_is_running = true;
  m_hnz_thread = std::thread(&HNZ::serverHnzThread, this);
}

void HNZ::stop()
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::stop -";
  HnzUtility::log_info("%s Starting shutdown of HNZ north plugin", beforeLog.c_str());
  m_is_running = false;
  if (m_hnz_thread.joinable())
  {
    m_hnz_thread.join();
  }
}

void HNZ::reconfigure(const ConfigCategory &config)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::reconfigure -";

  HnzUtility::log_info("%s Reconfigure plugin", beforeLog.c_str());
  if (!config.itemExists(PROTOCOL_STACK))
  {
    HnzUtility::log_error("%s Missing protocol_stack configuration", beforeLog.c_str());
    return;
  }

  if (!config.itemExists(EXCHANGED_DATA))
  {
    HnzUtility::log_error("%s Missing exchanged_data configuration", beforeLog.c_str());
    return;
  }

  const std::string protocolStack = config.getValue(PROTOCOL_STACK);

  const std::string dataExchange = config.getValue(EXCHANGED_DATA);

  try
  {
    m_hnz_config.reset(new HnzConfig(protocolStack, dataExchange));
    if (m_hnz_config->IsProtocolConfigComplete() == false && m_hnz_config->IsExchangeConfigComplete())
    {
      throw std::invalid_argument("bad protocol configuration");
    }
    stop();
    start();
  }
  catch (const std::invalid_argument &e)
  {
    HnzUtility::log_error("%s %s ", beforeLog.c_str(), e.what());
  }
}

void HNZ::setReadingParameters(ReadingParameters &params, Datapoint *const dp) const
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::setReadingParameters -";
  if (dp->getData().getType() == DatapointValue::T_STRING)
  {
    const auto value = dp->getData().toStringValue();
    if (dp->getName() == "do_type" || dp->getName() == "co_type")
    {
      params.msg_code = value;
    }
    else if (dp->getName() == "do_an")
    {
      params.an = value;
    }
    else
    {
      HnzUtility::log_warn("%s reading parameters string is unknown %s", beforeLog.c_str(), dp->getName().c_str());
    }
  }
  else if (dp->getData().getType() == DatapointValue::T_INTEGER)
  {
    const auto value = static_cast<unsigned int>(dp->getData().toInt());
    if (dp->getName() == "do_station")
    {
      params.station_addr = value;
    }
    else if (dp->getName() == "do_addr")
    {
      params.msg_address = value;
    }
    else if (dp->getName() == "do_value")
    {
      params.value = value;
    }
    else if (dp->getName() == "do_valid")
    {
      params.valid = value;
    }
    else if (dp->getName() == "do_cg")
    {
      params.cg = value;
    }
    else if (dp->getName() == "do_outdated")
    {
      params.outdated = value;
    }
    else if (dp->getName() == "do_ts")
    {
      params.ts = value;
    }
    else if (dp->getName() == "do_ts_iv")
    {
      params.ts_iv = value;
    }
    else if (dp->getName() == "do_ts_c")
    {
      params.ts_c = value;
    }
    else if (dp->getName() == "do_ts_s")
    {
      params.ts_s = value;
    }
    else if (dp->getName() == "co_addr")
    {
      params.msg_address = value;
    }
    else if (dp->getName() == "co_value")
    {
      params.value = value;
    }
    else
    {
      HnzUtility::log_warn("%s reading parameters int is unknown %s", beforeLog.c_str(), dp->getName().c_str());
    }
  }
  else
  {
    HnzUtility::log_warn("%s reading parameters has a unknown type %s", beforeLog.c_str(), dp->getName().c_str());
  }
}

bool HNZ::readDataObject(Reading *reading)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::readDataObject -";

  const auto &dataPoints = reading->getReadingData();
  const auto &assetName = reading->getAssetName();

  HnzUtility::log_debug("%s asset name: %s", beforeLog.c_str(), assetName.c_str());
  HnzUtility::log_debug("%s original Reading: %s", beforeLog.c_str(), reading->toJSON().c_str());

  HnzUtility::log_info("%s Forward data_object %s", beforeLog.c_str(), assetName.c_str());
  ReadingParameters params;
  for (const auto dp : dataPoints)
  {
    if (dp == nullptr)
    {
      continue;
    }
    setReadingParameters(params, dp);
  }

  std::lock_guard<std::mutex> lock(m_data_mtx);

  if (params.msg_code == "TS")
  {
    m_parseTSCE(params);
  }
  else if (params.msg_code == "TVC")
  {
    m_parseATVC(params);
  }
  else if (params.msg_code == "TC")
  {
    m_parseTVC(params);
  }
  else if (params.msg_code == "TM")
  {
    m_parseTM(params);
  }
  else
  {
    HnzUtility::log_warn("%s unknown msg type %s", beforeLog.c_str(), params.msg_code.c_str());
    return false;
  }

  return true;
}

uint32_t HNZ::send(std::vector<Reading *> &readings)
{
  std::lock_guard<std::mutex> lock(m_read_mtx);
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::send -";
  HnzUtility::log_debug("%s Receving data: %d", beforeLog.c_str(), readings.size());

  const auto size_before_read = static_cast<uint32_t>(readings.size());

  readings.erase(std::remove_if(readings.begin(), readings.end(), [this](Reading *read)
                       {
      if (nullptr == read)
      {
        return false;
      }
      

      return readDataObject(read); 
      
  }), readings.end());


  const auto size = static_cast<uint32_t>(readings.size());
  return size_before_read - size;
}

void HNZ::m_parseTSCE(const ReadingParameters &params)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::m_parseTSCE -";
  HnzUtility::log_debug("%s parse datapoint", beforeLog.c_str());

  std::vector<unsigned char> data(5, 0);

  data[0] = TSCE_CODE;
  unsigned int msg_address = params.msg_address;

  //  AD0 + ADB
  data[1] = static_cast<unsigned char>((msg_address / 10));     // AD0
  data[2] |= static_cast<unsigned char>(msg_address % 10) << 5; // ADB

  //  E bit et V bit
  data[2] |= static_cast<unsigned char>((params.value & 0x1) << 3); // E bit
  data[2] |= static_cast<unsigned char>((params.valid & 0x1) << 4); // V bit

  //  HNV bit, S bit et C bit
  data[2] |= static_cast<unsigned char>((params.ts_iv & 0x1) << 2); // HNV bit
  data[2] |= static_cast<unsigned char>((params.ts_s & 0x1));       // S bit
  data[2] |= static_cast<unsigned char>((params.ts_c & 0x1) << 1);  // C bit

  //  timestamp
  data[3] = static_cast<unsigned char>((params.ts >> 8) & 0xFF); // MSB
  data[4] = static_cast<unsigned char>(params.ts & 0xFF);        // LSB

  m_hnz_data_to_send.push_back(data);

  PaHnzData::setTsValue(static_cast<unsigned char>(msg_address), params.value & 0x1, params.valid & 0x1);
}

void HNZ::m_parseATVC(const ReadingParameters &params)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::m_parseATVC -";
  HnzUtility::log_debug("%s parse datapoint", beforeLog.c_str());

  std::vector<unsigned char> data(4, 0);

  unsigned int msg_address = params.msg_address;

  if (!m_hnz_config)
  {
    HnzUtility::log_debug("%s No config", beforeLog.c_str());
    return;
  }

  data[0] = TVCACK_CODE;
  //  AD0
  data[1] = static_cast<unsigned char>(msg_address & 0x1F);

  //  A bit
  data[1] |= static_cast<unsigned char>((params.valid & 0x1) << 6);

  long int value = params.value;
  if (value < 0)
  {
    value *= -1;
    data[3] |= 0x80;
  }
  data[2] = static_cast<unsigned char>(value & 0x7F);

  m_hnz_data_to_send.push_back(data);
}

void HNZ::m_parseTVC(const ReadingParameters &params)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::m_parseTVC -";
  HnzUtility::log_debug("%s parse datapoint", beforeLog.c_str());

  std::vector<unsigned char> data(3, 0);

  data[0] = TCACK_CODE;

  unsigned int msg_address = params.msg_address;

  //  AD0
  data[1] = static_cast<unsigned char>(msg_address / 10);

  //  ADB
  data[2] |= static_cast<unsigned char>((msg_address % 10)) << 5;

  // value
  if (params.value == 0)
  {
    data[2] |= static_cast<unsigned char>(0b1) << 3;
  }
  else
  {
    data[2] |= static_cast<unsigned char>(0b10) << 3;
  }

  // CR
  unsigned int CR = (params.valid == 0) ? 0x1 : 0x0;
  data[2] |= static_cast<unsigned char>(CR & 0b111);

  m_hnz_data_to_send.push_back(data);
}

void HNZ::m_parseTM(const ReadingParameters &params)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::m_parseTM -";
  HnzUtility::log_debug("%s parse datapoint", beforeLog.c_str());

  std::vector<unsigned char> data(6, 0);

  data[0] = TM4_CODE;

  //  addr
  data[1] = static_cast<unsigned char>((params.msg_address / 4) * 4);

  size_t index = (params.msg_address % 4);
  for (size_t i = 2; i < 6; i++)
  {
    if (i == index + 2)
    {
      data[i] = static_cast<unsigned char>(params.value);
    }
    else
    {
      data[i] = 0xff;
    }
  }

  m_hnz_data_to_send.push_back(data);
}

/* Utility function  */
static std::string paramsToStr(char **params, int count)
{
  std::string out("[");
  for (int i = 0; i < count; i++)
  {
    if (i > 0)
    {
      out += ", ";
    }
    out += "\"";
    out += params[i];
    out += "\"";
  }
  out += "]";
  return out;
}

int HNZ::operation(char *operation, int paramCount, char *names[], char *parameters[])
{

  if (!m_hnz_config)
  {
    return -1;
  }

  std::string beforeLog = HnzUtility::PluginName + " - HNZ::operation -";
  std::string namesStr = paramsToStr(names, paramCount);
  std::string paramsStr = paramsToStr(parameters, paramCount);
  HnzUtility::log_info(R"(%s Sending operation: {type: "%s", nbParams=%d, names=%s, parameters=%s, cmdDest="%s"})",
                       beforeLog.c_str(), operation, paramCount, namesStr.c_str(), paramsStr.c_str(), m_hnz_config->CmdDest().c_str());

  if (m_oper == nullptr)
  {
    HnzUtility::log_error("%s No operation callback available -> abort (registerControl must be called first)",
                          beforeLog.c_str());
    return -1;
  }

  if (!m_hnz_config->IsProtocolConfigComplete() || !m_hnz_config->IsExchangeConfigComplete())
  {
    HnzUtility::log_error("%s No config available -> abort", beforeLog.c_str());
    return -1;
  }

  int res = -1;
  if (m_hnz_config->CmdDest() == "")
  {
    res = m_oper(operation, paramCount, names, parameters, DestinationBroadcast, nullptr);
  }
  else
  {
    res = m_oper(operation, paramCount, names, parameters, DestinationService, m_hnz_config->CmdDest().c_str());
  }
  HnzUtility::log_debug("%s Operation returned %d", beforeLog.c_str(), res);
  // In the meantime consider they always succeed by returning 1 instead of res
  // return res;*/
  return 1;
}

void HNZ::registerControl(int (*operation)(char *operation, int paramCount, char *names[], char *parameters[], ControlDestination destination, ...))
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::registerControl -";

  m_oper = operation;

  HnzUtility::log_info("%s New operation callback registered", beforeLog.c_str());
}

char **HNZ::getCommandValues(size_t array_size, size_t string_size)
{
  if (array_size == 0 || string_size == 0)
  {
    return nullptr;
  }

  auto **command = new char *[array_size];
  for (size_t i = 0; i < array_size; i++)
  {
    command[i] = new char[string_size];
  }
  return command;
}

void HNZ::cleanCommandValues(char **command, size_t size)
{
  for (size_t i = 0; i < size; i++)
  {
    delete[] command[i];
    command[i] = nullptr;
  }
  delete[] command;
}

void HNZ::setCommandValue(char *command, const std::string &value, size_t max_size) const
{

  const auto value_length = value.length();
  const auto llength = std::min(value_length, max_size - 1);
  for (size_t i = 0; i < llength; i++)
  {
    command[i] = value[i];
  }
  command[llength] = '\0';
}

void HNZ::manageReceivedFrames(const std::vector<std::shared_ptr<MSG_TRAME>> &receivedFrames, const std::shared_ptr<EnhancedHNZServer> &server)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::manageReceivedFrames -";
  for (const auto &frame : receivedFrames)
  {
    if (frame && frame->usLgBuffer > 3)
    {
      // CG code
      if (frame->usLgBuffer > 3 && frame->aubTrame[2] == 0x13 && frame->aubTrame[3] == 0x1)
      {
        HnzUtility::log_info("%s Send CG", beforeLog.c_str());
        server->sendInformation(server->getStateMachine()->getHnzData().tsValueToTSCG(),
                                false);
        server->getStateMachine()->setCgSent(true);
      }
      // tc
      else if (frame->aubTrame[2] == TC_CODE)
      {
        HnzUtility::log_info("%s Send TC command", beforeLog.c_str());
        const auto ado = frame->aubTrame[3];
        const auto adb = (frame->aubTrame[4] & 0b11100000) >> 5;
        int address = adb + (ado << 8);
        const auto open_order = ((frame->aubTrame[4] & 0b11000) >> 3) == 0b10;

        size_t array_size = 3;
        size_t string_size = 255;
        auto commandNames = getCommandValues(array_size, string_size);

        setCommandValue(commandNames[0], "co_type", string_size);
        setCommandValue(commandNames[1], "co_addr", string_size);
        setCommandValue(commandNames[2], "co_value", string_size);

        auto commandParams = getCommandValues(array_size, string_size);

        setCommandValue(commandParams[0], "TC", string_size);
        setCommandValue(commandParams[1], std::to_string(address).c_str(), string_size);
        setCommandValue(commandParams[2], open_order ? "1" : "0", string_size);

        operation((char *)"HNZCommand", array_size, commandNames, commandParams);
        cleanCommandValues(commandNames, array_size);
        cleanCommandValues(commandParams, array_size);
      }
      // tvc
      else if (frame->aubTrame[2] == TVC_CODE)
      {
        HnzUtility::log_info("%s Send TVC command", beforeLog.c_str());
        const auto address = frame->aubTrame[3];

        const auto open_order = (frame->aubTrame[4] >> 3) == 0b10;

        size_t array_size = 3;
        size_t string_size = 255;

        auto commandNames = getCommandValues(array_size, string_size);

        setCommandValue(commandNames[0], "co_type", string_size);
        setCommandValue(commandNames[1], "co_addr", string_size);
        setCommandValue(commandNames[2], "co_value", string_size);

        auto commandParams = getCommandValues(3, string_size);

        setCommandValue(commandParams[0], "TVC", string_size);
        setCommandValue(commandParams[1], std::to_string(address).c_str(), string_size);
        setCommandValue(commandParams[2], open_order ? "1" : "0", string_size);

        operation((char *)"HNZCommand", 3, commandNames, commandParams);
        cleanCommandValues(commandNames, array_size);
        cleanCommandValues(commandParams, array_size);
      }
    }
  }
}

void HNZ::sendWaitingData(const std::shared_ptr<EnhancedHNZServer> &server)
{
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::sendWaitingData -";
  // send value
  std::lock_guard<std::mutex> lock(m_data_mtx);
  if (server->getStateMachine()->isCGSent() && !m_hnz_data_to_send.empty())
  {
    HnzUtility::log_debug("%s Send %d TIs to HNZ Centre", beforeLog.c_str(), m_hnz_data_to_send.size());
    for (const auto &data_to_send : m_hnz_data_to_send)
    {
      server->sendFrame(data_to_send, false);
      std::this_thread::sleep_for(std::chrono::milliseconds(PIVOT_TO_HNZ_CENTER_DURATION));
    }

    m_hnz_data_to_send.clear();
  }
}

void HNZ::managerServer(const std::shared_ptr<EnhancedHNZServer> &server)
{
  // running
  std::string beforeLog = HnzUtility::PluginName + " - HNZ::managerServer -";
  HnzUtility::log_info("%s Starting hnz servers on port %d", beforeLog.c_str(), server->getPort());

  server->startHNZServer();
  while (m_is_running)
  {
    if (server->ServerIsReady(SERVER_IS_READY_DURATION))
    {
      const auto receivedFrames = server->popLastFramesReceived();

      manageReceivedFrames(receivedFrames, server);
      sendWaitingData(server);
    }

    if (server->isRunning() == false && m_is_running == true)
    {
      server->stopHNZServer();
      server->startHNZServer();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SERVER_SLEEPING_DURATION_IN_MS));
  }

  server->stopHNZServer();
}

void HNZ::serverHnzThread()
{
  if (!m_hnz_config)
  {
    return;
  }
  ServersWrapper wrapper(m_hnz_config->GetTcpPortA(), m_hnz_config->GetTcpPortB());

  auto server1 = wrapper.server1();
  auto server2 = wrapper.server2();
  server1->getStateMachine()->setConfig(m_hnz_config);
  server2->getStateMachine()->setConfig(m_hnz_config);

  std::thread server_thread_1(&HNZ::managerServer, this, server1);

  std::thread server_thread_2(&HNZ::managerServer, this, server2);

  while (m_is_running)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SERVER_SLEEPING_DURATION_IN_MS));
  }
  server1->stopHNZServer();
  server2->stopHNZServer();
  server_thread_1.join();
  server_thread_2.join();
}
