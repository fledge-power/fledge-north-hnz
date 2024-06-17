#include <gtest/gtest.h>
#include "hnz.h"

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
   "remote_station_addr": 12,
   "inacc_timeout": 180,
   "max_sarm": 30,
   "repeat_path_A": 3,
   "repeat_path_B": 3,
   "repeat_timeout": 3000,
   "anticipation_ratio": 3,
   "test_msg_send": "1304",
   "test_msg_receive": "1304",
   "gi_schedule": "99:99",
   "gi_repeat_count": 3,
   "gi_time": 255,
   "c_ack_time": 10,
   "cmd_recv_timeout": 100000
  }
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


TEST(HNZFile, TestHNZFile) {
  HNZ server;
  auto command = server.getCommandValues(0,3);
  ASSERT_EQ(command, nullptr);
  command = server.getCommandValues(3,0);
  ASSERT_EQ(command, nullptr);
  command = server.getCommandValues(3,255);
  ASSERT_NE(command, nullptr);
  server.setCommandValue(command[0], "test", 255);
  std::string t (command[0]);
  ASSERT_EQ(std::string(command[0]), "test");
  server.cleanCommandValues(command, 3);

}




