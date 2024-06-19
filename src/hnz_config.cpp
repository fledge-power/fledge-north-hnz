#include <sstream>
#include <algorithm>

#include <arpa/inet.h>

#include "hnzutility.h"
#include "hnz_config.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

HnzConfig::HnzConfig(const std::string &protocolConfig,
 const std::string &exchangeConfig) 
{
    ImportProtocolConfig(protocolConfig);
    ImportExchangeConfig(exchangeConfig);
}

HnzConfig::SouthPluginMonitor::SouthPluginMonitor(const std::string &assetName) : m_assetName(assetName)
{

}

bool HnzConfig::m_isValidIPAddress(const std::string &addrStr)
{
    // see https://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, addrStr.c_str(), &(sa.sin_addr));

    return (result == 1);
}

bool HnzConfig::m_isValidTcpPort(int tcpPort) const
{
    return (tcpPort > 0 && tcpPort < 65536);
}

void HnzConfig::ImportProtocolConfig(const std::string &protocolConfig)
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::importProtocolConfig -";
    m_protocolConfigComplete = false;

    rapidjson::Document document;

    if (document.Parse(protocolConfig.c_str()).HasParseError())
    {
        HnzUtility::log_fatal("%s Parsing error in protocol_stack json, offset %u: %s", beforeLog.c_str(),
                              static_cast<unsigned>(document.GetErrorOffset()), GetParseError_En(document.GetParseError()));
        return;
    }

    if (!document.IsObject())
    {
        HnzUtility::log_fatal("%s Root is not an object", beforeLog.c_str());
        return;
    }

    if (!document.HasMember(PROTOCOL_STACK) || !document[PROTOCOL_STACK].IsObject())
    {
        HnzUtility::log_fatal("%s protocol_stack does not exist or is not an object", beforeLog.c_str());
        return;
    }

    const auto &protocolStack = document[PROTOCOL_STACK];

    if (!protocolStack.HasMember(TRANSPORT_LAYER) || !protocolStack[TRANSPORT_LAYER].IsObject())
    {
        HnzUtility::log_fatal("%s transport_layer does not exist or is not an object", beforeLog.c_str());
        return;
    }

    if (!protocolStack.HasMember(APPLICATION_LAYER) || !protocolStack[APPLICATION_LAYER].IsObject())
    {
        HnzUtility::log_fatal("%s application_layer does not exist or is not an object", beforeLog.c_str());
        return;
    }

    const auto &transportLayer = protocolStack[TRANSPORT_LAYER];
    const auto &applicationLayer = protocolStack[APPLICATION_LAYER];


    if (transportLayer.HasMember(PORT_PATH_A))
    {
        if (transportLayer[PORT_PATH_A].IsInt())
        {
            int tcpPort = transportLayer[PORT_PATH_A].GetInt();
            if (m_isValidTcpPort(tcpPort))
            {
                m_tcpPortA = tcpPort;
            }
            else
            {
                HnzUtility::log_warn("%s transport_layer.port_path_a value out of range [1..65535]: %d -> using default port (%d)",
                                     beforeLog.c_str(), tcpPort, m_tcpPortADefault);
            }
        }
        else
        {
            HnzUtility::log_warn("%s transport_layer.port_path_a in not an integer -> using default port (%d)", beforeLog.c_str(),
                                 m_tcpPortADefault);
        }
    }

    if (transportLayer.HasMember(PORT_PATH_B))
    {
        if (transportLayer[PORT_PATH_B].IsInt())
        {
            int tcpPort = transportLayer[PORT_PATH_B].GetInt();
            if (m_isValidTcpPort(tcpPort))
            {
                m_tcpPortB = tcpPort;
            }
            else
            {
                HnzUtility::log_warn("%s transport_layer.port_path_b value out of range [1..65535]: %d -> using default port (%d)",
                                     beforeLog.c_str(), tcpPort, m_tcpPortBDefault);
            }
        }
        else
        {
            HnzUtility::log_warn("%s transport_layer.port_path_b in not an integer -> using default port (%d)", beforeLog.c_str(),
                                 m_tcpPortBDefault);
        }
    }

    if(!m_importApplicationLayer(applicationLayer)) {
        HnzUtility::log_warn("%s Application layed failed (%d)", beforeLog.c_str());
        return;
    }

    m_protocolConfigComplete = true;
}

void HnzConfig::ImportExchangeConfig(const std::string &exchangeConfig)
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::importExchangedDataJson - ";
    m_exchangeConfigComplete = false;
    bool is_complete = true;

    rapidjson::Document document;
    if (document.Parse(exchangeConfig.c_str()).HasParseError())
    {
        HnzUtility::log_fatal(beforeLog + "Parsing error in exchanged_data json, offset " +
                              std::to_string((unsigned)document.GetErrorOffset()) +
                              " " +
                              GetParseError_En(document.GetParseError()));
        return;
    }
    if (!document.IsObject())
        return;

    if (!m_check_object(document, EXCHANGED_DATA))
        return;

    const auto &info = document[EXCHANGED_DATA];

    is_complete &= m_check_string(info, NAME);
    is_complete &= m_check_string(info, JSON_VERSION);

    if (!m_check_array(info, DATAPOINTS))
        return;

    m_lastTSAddr = 0;
    for (const auto &msg : info[DATAPOINTS].GetArray())
    {
        is_complete &= m_importDatapoint(msg);
    }

    m_exchangeConfigComplete = is_complete;
}

bool HnzConfig::m_importDatapoint(const rapidjson::Value &msg)
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_importDatapoint - ";

    if (!msg.IsObject())
        return false;

    std::string label;

    bool is_complete = true;
    is_complete &= m_retrieve(msg, LABEL, &label);
    is_complete &= m_check_string(msg, PIVOT_ID);
    is_complete &= m_check_string(msg, PIVOT_TYPE);

    if (!m_check_array(msg, PROTOCOLS))
        return false;

    for (const auto &protocol : msg[PROTOCOLS].GetArray())
    {
        if (!protocol.IsObject())
            return false;

        std::string protocol_name;

        is_complete &= m_retrieve(protocol, NAME, &protocol_name);

        if (protocol_name != HNZ_NAME)
            continue;

        std::string address;
        std::string msg_code;

        is_complete &= m_retrieve(protocol, MESSAGE_ADDRESS, &address, "");
        is_complete &= m_retrieve(protocol, MESSAGE_CODE, &msg_code, "");

        unsigned long tmp = 0;
        try
        {
            tmp = std::stoul(address);
        }
        catch (const std::invalid_argument &e)
        {
            HnzUtility::log_error(beforeLog + "Cannot convert address '" + address + "' to integer: " + typeid(e).name() + ": " + e.what());
            is_complete = false;
        }
        catch (const std::out_of_range &e)
        {
            HnzUtility::log_error(beforeLog + "Cannot convert address '" + address + "' to integer: " + typeid(e).name() + ": " + e.what());
            is_complete = false;
        }
        unsigned int msg_address = 0;
        // Check if number is in range for unsigned int
        if (tmp > static_cast<unsigned int>(-1))
        {
            is_complete = false;
        }
        else
        {
            msg_address = static_cast<unsigned int>(tmp);
        }
        m_msg_list[msg_code][m_remote_station_addr][msg_address] = label;
        if (msg_address > m_lastTSAddr)
        {
            m_lastTSAddr = msg_address;
        }
    }
    return is_complete;
}

bool HnzConfig::m_importApplicationLayer(const rapidjson::Value &conf)
{


    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_importApplicationLayer - ";
    bool is_complete = true;
    unsigned int get_value = 0;
    is_complete &= m_retrieve(conf, REMOTE_ADDR, &get_value);
    if (get_value > 64)
    {
        HnzUtility::log_error("%s Error with the field %s, the value is not on 6 bits.",beforeLog.c_str(), REMOTE_ADDR);
        is_complete = false;
        return is_complete;
    } else {
        m_remote_station_addr = static_cast<unsigned char>(get_value);
    }

    is_complete &= m_retrieve(conf, INACC_TIMEOUT, &m_inacc_timeout);
    is_complete &= m_retrieve(conf, MAX_SARM, &m_max_sarm);
    is_complete &= m_retrieve(conf, REPEAT_PATH_A, &m_repeat_path_A);
    is_complete &= m_retrieve(conf, REPEAT_PATH_B, &m_repeat_path_B);
    is_complete &= m_retrieve(conf, TST_MSG_SEND, &m_test_msg_send,"");
    is_complete &= m_retrieve(conf, TST_MSG_RECEIVE, &m_test_msg_receive,"");
    is_complete &= m_retrieve(conf, GI_SCHEDULE, &m_gi_schedule,"");
    is_complete &= m_retrieve(conf, REPEAT_TIMEOUT, &m_repeat_timeout);
    is_complete &= m_retrieve(conf, ANTICIPATION_RATIO, &m_anticipation_ratio);
    is_complete &= m_retrieve(conf, GI_REPEAT_COUNT, &m_gi_repeat_count);
    is_complete &= m_retrieve(conf, GI_TIME, &m_gi_time);
    is_complete &= m_retrieve(conf, C_ACK_TIME, &m_c_ack_time);
    is_complete &= m_retrieve(conf, CMD_RECV_TIMEOUT, &m_cmd_recv_timeout);
    is_complete &= m_retrieve(conf, CMD_DEST, &m_cmd_dest,"");

    return is_complete;
}

bool HnzConfig::m_retrieve(const rapidjson::Value &json, const char *key, unsigned int *target) const
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_retrieve - ";
    if (!json.HasMember(key) || !json[key].IsUint())
    {
        HnzUtility::log_error(beforeLog + "Error with the field " + key + ", the value does not exist or is not an unsigned integer.");
        return false;
    }
    *target = json[key].GetUint();
    return true;
}

bool HnzConfig::m_retrieve(const rapidjson::Value &json, const char *key, unsigned int *target, unsigned int def) const
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_retrieve - ";
    if (!json.HasMember(key))
    {
        *target = def;
    }
    else
    {
        if (!json[key].IsUint())
        {
            HnzUtility::log_error(beforeLog + "Error with the field " + key + ", the value is not an unsigned integer.");
            return false;
        }
        *target = json[key].GetUint();
    }
    return true;
}

bool HnzConfig::m_retrieve(const rapidjson::Value &json, const char *key, std::string *target) const
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_retrieve - ";
    if (!json.HasMember(key) || !json[key].IsString())
    {
        HnzUtility::log_error(beforeLog + "Error with the field " + key + ", the value does not exist or is not a string.");
        return false;
    }
    *target = json[key].GetString();
    return true;
}

bool HnzConfig::m_retrieve(const rapidjson::Value &json, const char *key, std::string *target, const std::string &def) const
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_retrieve - ";
    if (!json.HasMember(key))
    {
        *target = def;
    }
    else
    {
        if (!json[key].IsString())
        {
            HnzUtility::log_error(beforeLog + "Error with the field " + key + ", the value is not a string.");
            return false;
        }
        *target = json[key].GetString();
    }
    return true;
}

int HnzConfig::GetTcpPortA() const
{
    if (m_tcpPortA == -1)
    {
        return m_tcpPortADefault;
    }
    return m_tcpPortA;
}

int HnzConfig::GetTcpPortB() const
{
    if (m_tcpPortB == -1)
    {
        return m_tcpPortBDefault;
    }
    return m_tcpPortB;
}

std::string valueToString(const rapidjson::Value& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

bool HnzConfig::m_check_object(const rapidjson::Value &json, const char *key) const
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_check_object - ";
    if (!json.HasMember(key) || !json[key].IsObject())
    {
        HnzUtility::log_error(beforeLog + "The object " + key + " is required but not found in "+ valueToString(json) + ".");
        return false;
    }
    return true;
}

bool HnzConfig::m_check_string(const rapidjson::Value &json, const char *key) const
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_check_string - ";
    if (!json.HasMember(key) || !json[key].IsString())
    {
        HnzUtility::log_error(beforeLog + "Error with the field " + key + ", the value does not exist or is not a string.");
        return false;
    }
    return true;
}

bool HnzConfig::m_check_array(const rapidjson::Value &json, const char *key) const
{
    std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::m_check_array - ";
    if (!json.HasMember(key) || !json[key].IsArray())
    {

        HnzUtility::log_error(beforeLog + "The array " + key + " is required but not found.");
        return false;
    }
    return true;
}

std::string HnzConfig::GetLabel(const std::string &msg_code, const int msg_address) const {
  std::string beforeLog = HnzUtility::PluginName + " - HnzConfig::getLabel - ";
  std::string label;
  try {
    label = m_msg_list.at(msg_code).at(m_remote_station_addr).at(msg_address);
  } catch (const std::out_of_range &e) {
    std::string code = MESSAGE_CODE;
    std::string st_addr = REMOTE_ADDR;
    std::string msg_addr = MESSAGE_ADDRESS;
    label = "";
    HnzUtility::log_warn(beforeLog +
        "The message received does not exist in the configuration (" + code +
        " : " + msg_code + ", " + st_addr + " : " +
        std::to_string(m_remote_station_addr) + " and " + msg_addr + " : " +
        std::to_string(msg_address) + "). : %s", e.what());
  }
  return label;
}
