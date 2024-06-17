#include <gtest/gtest.h>
#include "hnz_config.h"

const std::string protocol_stack = R"(
  {
 "protocol_stack": {
  "name": "hnzclient",
  "version": "1.0",
  "transport_layer": {
   "port_path_A": 6001,
   "port_path_B": 6002
  },
  "application_layer": {
    "remote_station_addr":12,
    "inacc_timeout":180,
    "max_sarm":30,
    "repeat_path_A":3,
    "repeat_path_B":3,
    "repeat_timeout":3000,
    "anticipation_ratio":3,
    "test_msg_send":"1304",
    "test_msg_receive":"1304",
    "gi_schedule":"99:99",
    "gi_repeat_count":3,
    "gi_time":255,
    "c_ack_time":10,
    "cmd_recv_timeout":100000
  }
 }
}
)";

const std::string protocol_stack_no_remote_address = R"(
  {
 "protocol_stack": {
  "name": "hnzclient",
  "version": "1.0",
  "transport_layer": {
   "port_path_A": 6001,
   "port_path_B": 6002
  },
  "application_layer": {
      "inacc_timeout":180,
      "max_sarm":30,
      "repeat_path_A":3,
      "repeat_path_B":3,
      "repeat_timeout":3000,
      "anticipation_ratio":3,
      "test_msg_send":"1304",
      "test_msg_receive":"1304",
      "gi_schedule":"99:99",
      "gi_repeat_count":3,
      "gi_time":255,
      "c_ack_time":10,
      "cmd_recv_timeout":100000
  }
 }
}
)";

const std::string protocol_stack_bad_remote_address = R"(
  {
 "protocol_stack": {
  "name": "hnzclient",
  "version": "1.0",
  "transport_layer": {
   "port_path_A": 6001,
   "port_path_B": 6002
  },
  "application_layer": {
      "remote_station_addr":95,
      "inacc_timeout":180,
      "max_sarm":30,
      "repeat_path_A":3,
      "repeat_path_B":3,
      "repeat_timeout":3000,
      "anticipation_ratio":3,
      "test_msg_send":"1304",
      "test_msg_receive":"1304",
      "gi_schedule":"99:99",
      "gi_repeat_count":3,
      "gi_time":255,
      "c_ack_time":10,
      "cmd_recv_timeout":100000
  }
 }
}
)";

const std::string protocol_stack_bad_array = R"(
  {
 "protocol_stack": {
  "name": "hnzclient",
  "version": "1.0",
  "transport_layer": [],
  "application_layer": []
 }
}
)";

const std::string exchanged_data = R"(
{
 "exchanged_data": {
  "name": "TRESC",
  "version": "1.0",
  "datapoints": [
   {
    "label": "SI_PRT.INFO",
    "pivot_id": "S88194681",
    "pivot_type": "SpsTyp",
    "protocols": [
     {
      "name": "hnzip",
      "typeid": "TS",
      "address": "0020",
      "station": "12"
     },
     {
      "name": "iec104",
      "address": "16837-487424",
      "typeid": "M_SP_TB_1",
      "gi_groups": "station"
     }
    ]
   }
  ]
 }
}
)";

const std::string exchanged_data_array = R"(
{
 "exchanged_data": []
}
)";


TEST(HNZConfig, TestConfigOK) {
  
  HnzConfig config(protocol_stack, exchanged_data);
  ASSERT_EQ(config.IsProtocolConfigComplete(), true);
  ASSERT_EQ(config.IsExchangeConfigComplete(), true);

}

TEST(HNZConfig, TestConfigBAD) {
  
  HnzConfig config(protocol_stack.substr(20), exchanged_data.substr(20));
  ASSERT_EQ(config.IsProtocolConfigComplete(), false);
  ASSERT_EQ(config.IsExchangeConfigComplete(), false);

}

TEST(HNZConfig, TestConfigEmpty) {
  
  HnzConfig config("", exchanged_data.substr(20));
  ASSERT_EQ(config.IsProtocolConfigComplete(), false);
  ASSERT_EQ(config.IsExchangeConfigComplete(), false);

}

TEST(HNZConfig, TestConfigNodRemoteAddress) {

  HnzConfig config(protocol_stack_no_remote_address, exchanged_data);
  ASSERT_EQ(config.IsProtocolConfigComplete(), false);
  ASSERT_EQ(config.IsExchangeConfigComplete(), true);

}

TEST(HNZConfig, TestConfigBadRemoteAddress) {

  HnzConfig config(protocol_stack_bad_remote_address, exchanged_data);
  ASSERT_EQ(config.IsProtocolConfigComplete(), false);
  ASSERT_EQ(config.IsExchangeConfigComplete(), true);
}


TEST(HNZConfig, TestConfigArray) {

  HnzConfig config(protocol_stack_bad_array, exchanged_data_array);
  ASSERT_EQ(config.IsProtocolConfigComplete(), false);
  ASSERT_EQ(config.IsExchangeConfigComplete(), false);
}

TEST(HNZConfig, TestGetLabel) {

  HnzConfig config(protocol_stack, exchanged_data);
  ASSERT_EQ(config.GetLabel("12",13), "");
  ASSERT_EQ(config.GetLabel("TS",20), "SI_PRT.INFO");
}

TEST(HNZConfig, TestCmdDest) {

  HnzConfig config(protocol_stack, exchanged_data);
  ASSERT_EQ(config.CmdDest(), "");

}


TEST(HNZConfig, SouthPluginMonitor) {

  HnzConfig::SouthPluginMonitor monitor("label");
  ASSERT_EQ(monitor.GetAssetName(),"label");
  ASSERT_NO_THROW(monitor.SetConnxStatus(HnzConfig::ConnectionStatus::STARTED));
  ASSERT_EQ(monitor.GetConnxStatus(),HnzConfig::ConnectionStatus::STARTED);
  ASSERT_NO_THROW(monitor.SetGiStatus(HnzConfig::GiStatus::IDLE));
  ASSERT_EQ(monitor.GetGiStatus(),HnzConfig::GiStatus::IDLE);
}