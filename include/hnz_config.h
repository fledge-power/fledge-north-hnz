#pragma once
#include <map>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>


constexpr char PROTOCOL_STACK[] = "protocol_stack";
constexpr char NAME[] = "name";
constexpr char JSON_VERSION[] = "version";
constexpr char APPLICATION_LAYER[] = "application_layer";
constexpr char TRANSPORT_LAYER[] = "transport_layer";
constexpr char CONNECTIONS[] = "connections";
constexpr char PORT_PATH_A[] = "port_path_A";
constexpr char PORT_PATH_B[] = "port_path_B";
constexpr char REMOTE_ADDR[] = "remote_station_addr";
constexpr char INACC_TIMEOUT[] = "inacc_timeout";
constexpr char MAX_SARM[] = "max_sarm";
constexpr char REPEAT_PATH_A[] = "repeat_path_A";
constexpr char REPEAT_PATH_B[] = "repeat_path_B";
constexpr char REPEAT_TIMEOUT[] = "repeat_timeout";
constexpr char ANTICIPATION_RATIO[] = "anticipation_ratio";
constexpr char TST_MSG_SEND[] = "test_msg_send";
constexpr char TST_MSG_RECEIVE[] = "test_msg_receive";
constexpr char GI_SCHEDULE[] = "gi_schedule";
constexpr char GI_REPEAT_COUNT[] = "gi_repeat_count";
constexpr char GI_TIME[] = "gi_time";
constexpr char C_ACK_TIME[] = "c_ack_time";
constexpr char CMD_RECV_TIMEOUT[] = "cmd_recv_timeout";
constexpr char CMD_DEST[] = "cmd_dest";
constexpr char SOUTH_MONITORING[] = "south_monitoring";
constexpr char SOUTH_MONITORING_ASSET[] = "asset";

constexpr char EXCHANGED_DATA[] = "exchanged_data";
constexpr char DATAPOINTS[] = "datapoints";
constexpr char LABEL[] = "label";
constexpr char PIVOT_ID[] = "pivot_id";
constexpr char PIVOT_TYPE[] = "pivot_type";
constexpr char PROTOCOLS[] = "protocols";
constexpr char HNZ_NAME[] = "hnzip";
constexpr char MESSAGE_CODE[] = "typeid";
constexpr char MESSAGE_ADDRESS[] = "address";

// Default value
constexpr unsigned int DEFAULT_PORT = 6001;
constexpr unsigned int DEFAULT_INACC_TIMEOUT = 180;
constexpr unsigned int DEFAULT_MAX_SARM = 30;
constexpr unsigned int DEFAULT_REPEAT_PATH = 3;
constexpr unsigned int DEFAULT_REPEAT_TIMEOUT = 3000;
constexpr unsigned int DEFAULT_ANTICIPATION_RATIO = 3;
constexpr char DEFAULT_TST_MSG[] = "1304";
constexpr char DEFAULT_GI_SCHEDULE[] = "99:99";
constexpr unsigned int DEFAULT_GI_REPEAT_COUNT = 3;
constexpr unsigned int DEFAULT_GI_TIME = 255;
constexpr unsigned int DEFAULT_C_ACK_TIME = 10;
constexpr long long int DEFAULT_CMD_RECV_TIMEOUT = 100000; // 100ms

constexpr int RETRY_CONN_DELAY = 5;

class HNZDataPoint;

class HnzConfig
{
public:
   HnzConfig(const std::string &protocolConfig, const std::string &exchangeConfig);


    void ImportProtocolConfig(const std::string &protocolConfig);
    void ImportExchangeConfig(const std::string &exchangeConfig);

    int GetTcpPortA() const;
    int GetTcpPortB() const; 

    enum class Mode
    {
        CONNECT_ALWAYS,
        CONNECT_IF_SOUTH_CONNX_STARTED
    };

    Mode GetMode() const { return m_mode; };

    enum class ConnectionStatus
    {
        STARTED,
        NOT_CONNECTED
    };

    enum class GiStatus
    {
        IDLE,
        STARTED,
        IN_PROGRESS,
        FAILED,
        FINISHED
    };

    class SouthPluginMonitor
    {

    public:
        explicit SouthPluginMonitor(const std::string &assetName);

        std::string &GetAssetName()  { return m_assetName; } ;

        ConnectionStatus GetConnxStatus() const { return m_connxStatus; };
        GiStatus GetGiStatus() const { return m_giStatus; };

        void SetConnxStatus(ConnectionStatus status) { m_connxStatus = status; };
        void SetGiStatus(GiStatus status) { m_giStatus = status; };

    private:
        std::string m_assetName;
        ConnectionStatus m_connxStatus = ConnectionStatus::NOT_CONNECTED;
        GiStatus m_giStatus = GiStatus::IDLE;
    };


    unsigned char GetRemoteStationAddress() const { return m_remote_station_addr;}
    std::vector<SouthPluginMonitor *> GetMonitoredSouthPlugins() const { return m_monitoredSouthPlugins; };

    bool IsProtocolConfigComplete() const { return m_protocolConfigComplete; };
    bool IsExchangeConfigComplete() const { return m_exchangeConfigComplete; };
    std::string GetLabel(const std::string &msg_code, const int msg_address) const;

    std::string CmdDest() const { return m_cmd_dest;};


private:
    static bool m_isValidIPAddress(const std::string &addrStr);

    bool m_isValidTcpPort(int tcpPort) const;

    Mode m_mode = Mode::CONNECT_ALWAYS;

    bool m_protocolConfigComplete{false};
    bool m_exchangeConfigComplete{false};

    int m_tcpPortA = -1; /* use default port */
    int m_tcpPortB = -1; /* use default port */

    const int m_tcpPortADefault{9090};
    const int m_tcpPortBDefault{9091};
    std::vector<SouthPluginMonitor *> m_monitoredSouthPlugins;
    bool m_check_object(const rapidjson::Value &json, const char *key) const;
    bool m_check_string(const rapidjson::Value &json, const char *key) const;
    bool m_check_array(const rapidjson::Value &json, const char *key) const;
    bool m_importDatapoint(const rapidjson::Value &msg);
    bool m_importApplicationLayer(const rapidjson::Value &conf);
    bool m_retrieve(const rapidjson::Value &json, const char *key, unsigned int *target) const;
    bool m_retrieve(const rapidjson::Value &json, const char *key, unsigned int *target, unsigned int def) const;
    bool m_retrieve(const rapidjson::Value &json, const char *key, std::string *target) const;
    bool m_retrieve(const rapidjson::Value &json, const char *key, std::string *target, const std::string &def) const;
    unsigned int m_lastTSAddr = 0;
    // Nested map of msg_code, remote_station_addr and msg_address
    std::map<std::string, std::map<unsigned int, std::map<unsigned int, std::string>>> m_msg_list;
    unsigned char  m_remote_station_addr = 0;
    unsigned int m_inacc_timeout{0};
    unsigned int m_max_sarm{0};
    unsigned int m_repeat_path_A{0};
    unsigned int m_repeat_path_B{0};
    unsigned int m_repeat_timeout{0};
    unsigned int m_anticipation_ratio{0};
    std::string m_test_msg_send{};
    std::string m_test_msg_receive{};
    std::string m_gi_schedule{};
    unsigned int m_gi_repeat_count{0};
    unsigned int m_gi_time{0};
    unsigned int m_c_ack_time{0};
    unsigned int m_cmd_recv_timeout{0};
    std::string m_cmd_dest;
};
