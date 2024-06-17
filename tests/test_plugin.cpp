#include <config_category.h> 
#include <gtest/gtest.h>
#include <plugin_api.h>  
#include <reading.h>
#include <memory>
#include <string>
#include <utility>
#include <thread>
#include <vector>
#include <reading_set.h>
#include <rapidjson/document.h>

#include "hnz.h"

using namespace std;

typedef void (*INGEST_CB)(void *, Reading);

extern "C" {
PLUGIN_INFORMATION *plugin_info();
PLUGIN_HANDLE plugin_init(ConfigCategory *config);

void plugin_reconfigure(PLUGIN_HANDLE handle, string &newConfig);

void plugin_register(PLUGIN_HANDLE handle,
		bool ( *write)(const char *name, const char *value, ControlDestination destination, ...),
		int (* operation)(char *operation, int paramCount, char *names[], char *parameters[], ControlDestination destination, ...));
void plugin_shutdown(PLUGIN_HANDLE handle);
void plugin_start(PLUGIN_HANDLE handle);
int plugin_send(PLUGIN_HANDLE handle, std::vector<Reading *>& readings);
};


// Dummy object used to be able to call parseJson() freely
static DatapointValue dummyValue("");
static Datapoint dummyDataPoint({}, dummyValue);

static void createReadingSet(ReadingSet*& outReadingSet, const std::string& assetName, const std::vector<std::string>& jsons)
{
    std::vector<Datapoint*> *allPoints = new std::vector<Datapoint*>();
    for(const std::string& json: jsons) {
        // Create Reading
        std::vector<Datapoint*> *p = nullptr;
        ASSERT_NO_THROW(p = dummyDataPoint.parseJson(json));
        ASSERT_NE(p, nullptr);
        allPoints->insert(std::end(*allPoints), std::begin(*p), std::end(*p));
        delete p;
    }
    Reading *reading = new Reading(assetName, *allPoints);
    std::vector<Reading*> *readings = new std::vector<Reading*>();
    readings->push_back(reading);
    // Create ReadingSet
    outReadingSet = new ReadingSet(readings);
}

static void createReadingSet(ReadingSet*& outReadingSet, const std::string& assetName, const std::string& json)
{
    createReadingSet(outReadingSet, assetName, std::vector<std::string>{json});
}

static const char *default_config = QUOTE(
{
    "plugin": {
        "description": "hnz north plugin",
        "type": "string",
        "default": "hnz",
        "readonly": "true"
    },
    "protocol_stack": {
        "description": "protocol stack parameters",
        "type": "JSON",
        "displayName": "Protocol stack parameters",
        "order": "2",
        "default": "{ \"protocol_stack\" : { \"name\" : \"hnzclient\", \"version\" : \"1.0\", \"transport_layer\" : { \"port_path_A\": 9090, \"port_path_B\": 9091 }, \"application_layer\" : { \"ca_asdu_size\":2, \"ioaddr_size\":3, \"asdu_size\":0, \"time_sync\":false, \"cmd_exec_timeout\":20, \"cmd_recv_timeout\":60, \"accept_cmd_with_time\":2, \"cmd_dest\": \"\", \"filter_orig\":false, \"filter_list\":[ { \"orig_addr\":0 }, { \"orig_addr\":1 }, { \"orig_addr\":2 } ] } } }",
        "mandatory": "true"
    },
    "exchanged_data": {
        "description": "exchanged data list",
        "type": "JSON",
        "displayName": "Exchanged data list",
        "order": "3",
        "default": "{ \"exchanged_data\" : { \"name\" : \"SAMPLE\", \"version\" : \"1.0\", \"datapoints\" : [ { \"label\" : \"TS1\", \"pivot_id\" : \"ID114562\", \"pivot_type\" : \"SpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-672\", \"typeid\" : \"M_SP_TB_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_StateQTimeTagExtended\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TS\" } ] }, { \"label\" : \"TM1\", \"pivot_id\" : \"ID99876\", \"pivot_type\" : \"DpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-984\", \"typeid\" : \"M_ME_NA_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_RealQ\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TM\" } ] } ] } }",
        "mandatory": "true"
    }
}
);

string new_test_conf = QUOTE(
{
    "plugin": {
        "description": "hnz north plugin",
        "type": "string",
        "default": "hnz",
        "readonly": "true"
    },
    "protocol_stack": {
        "description": "protocol stack parameters",
        "type": "JSON",
        "displayName": "Protocol stack parameters",
        "order": "2",
        "default": "{ \"protocol_stack\" : { \"name\" : \"hnzclient\", \"version\" : \"1.0\", \"transport_layer\" : { \"port_path_A\": 9092, \"port_path_B\": 9093 }, \"application_layer\" : { \"ca_asdu_size\":2, \"ioaddr_size\":3, \"asdu_size\":0, \"time_sync\":false, \"cmd_exec_timeout\":20, \"cmd_recv_timeout\":60, \"accept_cmd_with_time\":2, \"cmd_dest\": \"\", \"filter_orig\":false, \"filter_list\":[ { \"orig_addr\":0 }, { \"orig_addr\":1 }, { \"orig_addr\":2 } ] } } }",
        "mandatory": "true"
    },
    "exchanged_data": {
        "description": "exchanged data list",
        "type": "JSON",
        "displayName": "Exchanged data list",
        "order": "3",
        "default": "{ \"exchanged_data\" : { \"name\" : \"SAMPLE\", \"version\" : \"1.0\", \"datapoints\" : [ { \"label\" : \"TS1\", \"pivot_id\" : \"ID114562\", \"pivot_type\" : \"SpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-672\", \"typeid\" : \"M_SP_TB_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_StateQTimeTagExtended\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TS\" } ] }, { \"label\" : \"TM1\", \"pivot_id\" : \"ID99876\", \"pivot_type\" : \"DpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-984\", \"typeid\" : \"M_ME_NA_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_RealQ\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TM\" } ] } ] } }",
        "mandatory": "true"
    }
}
);


TEST(HNZ, PluginInit) {
  ConfigCategory *config = new ConfigCategory("Test_Config", default_config);
  config->setItemsValueFromDefault();
  PLUGIN_HANDLE handle = nullptr;

  ASSERT_NO_THROW(handle = plugin_init(config));
  ASSERT_NE(handle, nullptr);

  if (handle != nullptr) ASSERT_NO_THROW(plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle)));

  delete config;
}

TEST(HNZ, PluginInitNoConfig) {
  PLUGIN_HANDLE handle = nullptr;

  ASSERT_NO_THROW(handle = plugin_init(nullptr));
  ASSERT_NE(handle, nullptr);

  if (handle != nullptr) ASSERT_NO_THROW(plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle)));
}

TEST(HNZ, PluginStart) {
  ConfigCategory *emptyConfig = new ConfigCategory();
  PLUGIN_HANDLE handle = nullptr;
  ASSERT_NO_THROW(handle = plugin_init(emptyConfig));
  ASSERT_NE(handle, nullptr);
  ASSERT_NO_THROW(plugin_start(static_cast<PLUGIN_HANDLE *>(handle)));

  if (handle != nullptr) ASSERT_NO_THROW(plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle)));

  delete emptyConfig;
}

TEST(HNZ, PluginInitEmptyConfig) {
  ConfigCategory *emptyConfig = new ConfigCategory();
  PLUGIN_HANDLE handle = nullptr;
  ASSERT_NO_THROW(handle = plugin_init(emptyConfig));
  ASSERT_NE(handle, nullptr);

  if (handle != nullptr) ASSERT_NO_THROW(plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle)));

  delete emptyConfig;
}

TEST(HNZ, PluginReconfigure) {
  ConfigCategory *emptyConfig = new ConfigCategory();
  PLUGIN_HANDLE handle = nullptr;
  ASSERT_NO_THROW(handle = plugin_init(emptyConfig));
  ASSERT_NE(handle, nullptr);
  ASSERT_NO_THROW(plugin_reconfigure(handle, new_test_conf));

  ASSERT_NO_THROW(plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle)));

  ASSERT_THROW(plugin_reconfigure(nullptr, new_test_conf), exception);

  delete emptyConfig;
}

TEST(HNZ, PluginStop) {
  ConfigCategory *emptyConfig = new ConfigCategory();
  PLUGIN_HANDLE handle = nullptr;
  ASSERT_NO_THROW(handle = plugin_init(emptyConfig));
  ASSERT_NE(handle, nullptr);

  ASSERT_NO_THROW(plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle)));

  delete emptyConfig;
}

TEST(HNZ, PluginStopDuringConnect) {
  PLUGIN_INFORMATION *info = nullptr;
  ASSERT_NO_THROW(info = plugin_info());
  ConfigCategory *config = new ConfigCategory("default_config", info->config);
  config->setItemsValueFromDefault();
  PLUGIN_HANDLE handle = nullptr;

  ASSERT_NO_THROW(handle = plugin_init(config));
  ASSERT_NE(handle, nullptr);

  ASSERT_NO_THROW(plugin_start(static_cast<PLUGIN_HANDLE *>(handle)));

  // Wait for plugin to enter HNZPath::connect() on both path
  printf("Waiting for HNZPath::connect()...\n"); fflush(stdout);
  this_thread::sleep_for(chrono::seconds(10));

  // Make sure that shutdown does not hang forever on "hnz - HNZ::stop - Waiting for the receiving thread"
  printf("Waiting for plugin_shutdown() to complete...\n"); fflush(stdout);
  // plugin_shutdown() never return when run from GitHub CI when called from a sub-thread,
  // resulting in the test failing, so leave it in the main test thread at the risk of the whole test hanging
  // std::atomic<bool> shutdownSuccess{false};
  // std::thread joinThread([&handle, &shutdownSuccess](){
  //   plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle));
  //   shutdownSuccess = true;
  // });
  // joinThread.detach();
  // int timeMs = 0;
  // // Wait up to 60s for plugin_shutdown to complete
  // while (timeMs < 60000) {
  //   if (shutdownSuccess) {
  //     break;
  //   }
  //   this_thread::sleep_for(chrono::milliseconds(100));
  //   timeMs += 100;
  // }
  // ASSERT_TRUE(shutdownSuccess) << "Shutdown did not complete in time, it's probably hanging on a thread join()";
  ASSERT_NO_THROW(plugin_shutdown(static_cast<PLUGIN_HANDLE *>(handle)));

  delete config;
}

TEST(HNZ, plugin_ts_bad_send)
{

std::string jsonMessagePivotLabelMismatch = QUOTE(
{
    "PIVOT": {
        "GTIC": {
            "ComingFrom": "iec104",
            "SpcTyp": {
                "q": {
                    "test": 0
                },
                "t": {
                    "SecondSinceEpoch": 1718022207,
                    "FractionOfSecond": 12549357,
                    "TimeQuality": {
                        "clockFailure": 0,
                        "leapSecondKnown": 1,
                        "timeAccuracy": 10
                    }
                }
            },
            "Identifier": "C13331951",
            "Cause": {
                "stVal": 3
            },
            "Confirmation": {
                "stVal": 0
            },
            "TmOrg": {
                "stVal": "substituted"
            }
        }
    }
}

);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessagePivotLabelMismatch});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
  
}

TEST(HNZ, plugin_ts_send_no_label)
{

std::string jsonMessagePivotLabelMismatch = QUOTE(
{
    "data_object":{
        "do_type":"TS",
        "do_station":12,
        "do_addr":511,
        "do_value":1,
        "do_valid":0,
        "do_cg":0,
        "do_outdated":0,
        "do_ts": 1685019425432,
        "do_ts_iv":0,
        "do_ts_c":0,
        "do_ts_s":0
    }
}

);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessagePivotLabelMismatch});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
  
}

TEST(HNZ, plugin_send_ts_invalid)
{

std::string jsonMessagePivotLabelMismatch = QUOTE(
{
    "data_object":{

    }
}

);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessagePivotLabelMismatch});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
}

TEST(HNZ, plugin_ts_send_valid)
{

std::string jsonMessagePivotLabelMismatch = QUOTE(
{
    "data_object":{
        "do_type":"TS",
        "do_station":12,
        "do_addr":511,
        "do_value":1,
        "do_valid":0,
        "do_cg":0,
        "do_outdated":0,
        "do_ts": 1685019425432,
        "do_ts_iv":0,
        "do_ts_c":0,
        "do_ts_s":0
    }
}

);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessagePivotLabelMismatch});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
  
}

TEST(HNZ, plugin_ts_bad_cg_send_valid)
{

std::string jsonMessagePivotLabelMismatch = QUOTE(
{
    "data_object":{
        "do_type":"TS",
        "do_station":12,
        "do_addr":511,
        "do_value":1,
        "do_valid":0,
        "do_cg":1,
        "do_outdated":0,
        "do_ts": 1685019425432,
        "do_ts_iv":0,
        "do_ts_c":0,
        "do_ts_s":0
    }
}

);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessagePivotLabelMismatch});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
  
}


TEST(HNZ, plugin_tm_send_valid)
{

std::string jsonMessagePivotLabelMismatch = QUOTE(
{
    "data_object":{
        "do_type":"TM",
        "do_station":12,
        "do_addr":20,
        "do_value":-42,
        "do_valid":0,
        "do_an":"TMA",
        "do_outdated":0
    }
}

);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessagePivotLabelMismatch});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
  
}


TEST(HNZ, plugin_empty_data)
{
    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));
    std::vector<Reading*> readings;
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, readings));
  
}

TEST(HNZ, plugin_2_tm_ts)
{

std::string jsonMessagePivotTm = QUOTE(
{
    "data_object":{
        "do_type":"TM",
        "do_station":12,
        "do_addr":20,
        "do_value":-42,
        "do_valid":0,
        "do_an":"TMA",
        "do_outdated":0
    }
}

);

std::string jsonMessageTS = QUOTE(
{
    "data_object":{
        "do_type":"TS",
        "do_station":12,
        "do_addr":511,
        "do_value":1,
        "do_valid":0,
        "do_cg":0,
        "do_outdated":0,
        "do_ts": 1685019425432,
        "do_ts_iv":0,
        "do_ts_c":0,
        "do_ts_s":0
    }
}

);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessagePivotTm,jsonMessageTS});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
  
}

TEST(HNZ, plugin_TC)
{


std::string jsonMessageTC = QUOTE(
{
        "co_type":"TC",
        "co_addr":64,
        "co_value":0
}
);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessageTC});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_NO_THROW(plugin_send((PLUGIN_HANDLE)handle, *readings));
  
}

TEST(HNZ, plugin_TM)
{


std::string jsonMessageTM = QUOTE(
{
    "do_type": "TM",
    "do_station": 12,
    "do_addr": 4,
    "do_an": "TMA",
    "do_value": 1,
    "do_valid": 0,
    "do_outdated": 0
}
);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessageTM});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, *readings),1);
  
}


TEST(HNZ, plugin_ReadingParameter)
{


std::string jsonMessageTM = QUOTE(
{
    "do_type": "TM",
    "do_station": 12,
    "do_addr": 4,
    "do_an": "TMA",
    "do_value": 1,
    "do_valid": 0,
    "do_outdated": 0
}
);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessageTM});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    auto hnz = (HNZ*) handle;
    ReadingParameters params;
    const auto &dataPoints = (*readings)[0]->getReadingData();

    for (const auto dp : dataPoints)
    {
        if (dp == nullptr)
        {
        continue;
        }
        hnz->setReadingParameters(params, dp);
    }

    ASSERT_EQ(params.msg_code, "TM");
    ASSERT_EQ(params.station_addr, 12);
    ASSERT_EQ(params.msg_address, 4);
    ASSERT_EQ(params.value, 1);
    ASSERT_EQ(params.valid, 0);
}

TEST(HNZ, plugin_NO_TI)
{
    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    std::vector<Reading *> readings;
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, readings),0);
}

TEST(HNZ, plugin_TA)
{
std::string jsonMessageTM = QUOTE(
{
    "do_type": "TA",
    "do_station": 12,
    "do_addr": 4,
    "do_an": "TMA",
    "do_value": 1,
    "do_valid": 0,
    "do_outdated": 0
}
);




    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessageTM});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, *readings),0);
}

TEST(HNZ, plugin_empty)
{
    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    std::vector<Reading *> readings;
    readings.push_back(nullptr);
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, readings),0);

}

TEST(HNZ, plugin_THREE_TI)
{
    std::string jsonMessageTM = QUOTE(
{
    "do_type": "TM",
    "do_station": 12,
    "do_addr": 4,
    "do_an": "TMA",
    "do_value": 1,
    "do_valid": 0,
    "do_outdated": 0
}
);

std::string jsonMessageTC = QUOTE(
{
        "co_type":"TC",
        "co_addr":64,
        "co_value":0
}
);

std::string jsonMessageTVC = QUOTE(
{
        "co_type":"TVC",
        "co_addr":64,
        "co_value":0
}
);


    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    ReadingSet* readingSet1 = nullptr;
    ReadingSet* readingSet2 = nullptr;
    createReadingSet(readingSet, "PIVOT", {jsonMessageTM});
    createReadingSet(readingSet1, "PIVOT", {jsonMessageTC});
    createReadingSet(readingSet2, "PIVOT", {jsonMessageTVC});
    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    std::vector<Reading*>* readings1 = readingSet1->getAllReadingsPtr();
    std::vector<Reading*>* readings2 = readingSet2->getAllReadingsPtr();
    readings->insert( readings->end(), readings1->begin(), readings1->end() );
    readings->insert( readings->end(), readings2->begin(), readings2->end() );
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, *readings),3);

}

TEST(HNZ, plugin_TWO_TI)
{
    std::string jsonMessageTM = QUOTE(
{
    "do_type": "TM",
    "do_station": 12,
    "do_addr": 4,
    "do_an": "TMA",
    "do_value": 1,
    "do_valid": 0,
    "do_outdated": 0
}
);

std::string jsonMessageTC = QUOTE(
{
        "co_type":"TC",
        "co_addr":64,
        "co_value":0
}
);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    ReadingSet* readingSet1 = nullptr;

    createReadingSet(readingSet, "PIVOT", {jsonMessageTM});
    createReadingSet(readingSet1, "PIVOT", {jsonMessageTC});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    std::vector<Reading*>* readings1 = readingSet1->getAllReadingsPtr();

    readings->insert( readings->end(), readings1->begin(), readings1->end() );
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, *readings),2);

}

TEST(HNZ, plugin_TWO_TI_AND_ONE_EMPTY)
{
    std::string jsonMessageTM = QUOTE(
{
    "do_type": "TM",
    "do_station": 12,
    "do_addr": 4,
    "do_an": "TMA",
    "do_value": 1,
    "do_valid": 0,
    "do_outdated": 0
}
);

std::string jsonMessageTC = QUOTE(
{
        "co_type":"TC",
        "co_addr":64,
        "co_value":0
}
);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    ReadingSet* readingSet1 = nullptr;

    createReadingSet(readingSet, "PIVOT", {jsonMessageTM});
    createReadingSet(readingSet1, "PIVOT", {jsonMessageTC});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    std::vector<Reading*>* readings1 = readingSet1->getAllReadingsPtr();

    readings->insert( readings->end(), readings1->begin(), readings1->end() );
    readings->push_back(nullptr);
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, *readings),2);

}

TEST(HNZ, plugin_TWO_TI_AND_ONE_EMPTY_FIRST)
{
    std::string jsonMessageTM = QUOTE(
{
    "do_type": "TM",
    "do_station": 12,
    "do_addr": 4,
    "do_an": "TMA",
    "do_value": 1,
    "do_valid": 0,
    "do_outdated": 0
}
);

std::string jsonMessageTC = QUOTE(
{
        "co_type":"TC",
        "co_addr":64,
        "co_value":0
}
);

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;
    ReadingSet* readingSet1 = nullptr;

    createReadingSet(readingSet, "PIVOT", {jsonMessageTM});
    createReadingSet(readingSet1, "PIVOT", {jsonMessageTC});

    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();
    std::vector<Reading*>* readings1 = readingSet1->getAllReadingsPtr();

    readings->insert( readings->end(), readings1->begin(), readings1->end() );
    readings->insert(readings->begin(), nullptr);
    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, *readings),2);

}


TEST(HNZ, plugin_TS)
{
    std::string jsonStr = R"(
    {
            "do_type": "TS",
            "do_station":12,
            "do_addr":511,
            "do_value":1,
            "do_valid":0,
            "do_cg":0,
            "do_outdated":0,
            "do_ts": 1685019425432,
            "do_ts_iv":0,
            "do_ts_c":0,
            "do_ts_s":0
    }
    )";

    PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));

    ReadingSet* readingSet = nullptr;


    createReadingSet(readingSet, "PIVOT", {jsonStr});


    std::vector<Reading*>* readings = readingSet->getAllReadingsPtr();

    ASSERT_EQ(plugin_send((PLUGIN_HANDLE)handle, *readings),1);

}

TEST(HNZ, plugin_Operation)
{
     PLUGIN_HANDLE handle = nullptr;
    ASSERT_NO_THROW(handle = plugin_init(nullptr));
    ASSERT_TRUE(handle != nullptr);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE)handle, new_test_conf));
    ASSERT_NO_THROW(plugin_register((PLUGIN_HANDLE)handle, nullptr,nullptr));

}

