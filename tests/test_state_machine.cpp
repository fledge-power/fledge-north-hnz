#include <gtest/gtest.h>
#include "state_machine.h"
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


TEST(HNZStateMachine, TestStateMachineConnection) {
  StateMachine sm;
  ASSERT_EQ(sm.isConnected(), false);

  sm.receiveSARMCode();

  ASSERT_EQ(sm.isConnected(), false);

  sm.receiveUACode();
  ASSERT_EQ(sm.isConnected(), true);
}

TEST(HNZStateMachine, TestStateInformationNR) {
  StateMachine sm;
  ASSERT_EQ(sm.getControlInformation(0), 0);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b100000);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b1000000);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b1100000);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b10000000);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b10100000);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b11000000);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b11100000);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0);
  ASSERT_EQ(sm.getControlInformation(1), 0b10000);
}

TEST(HNZStateMachine, TestStateInformationNRNS) {
  StateMachine sm;
  ASSERT_EQ(sm.getControlInformation(0), 0);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b100010);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b1000100);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b1100110);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b10001000);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b10101010);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b11001100);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b11101110);
  ASSERT_EQ(sm.getControlInformation(1), 0b11111110);
  sm.receiveInformation();
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0);

}

TEST(HNZStateMachine, TestStateInformationNS) {
  StateMachine sm;
  ASSERT_EQ(sm.getControlInformation(0), 0);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b010);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b0100);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b0110);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b1000);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b01010);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b01100);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0b01110);
  sm.sendInformation();
  ASSERT_EQ(sm.getControlInformation(0), 0);
}

TEST(HNZStateMachine, TestStateAddress) {
  StateMachine sm;
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  ASSERT_EQ(config->IsProtocolConfigComplete(), true);
  ASSERT_EQ(config->IsExchangeConfigComplete(), true);
  sm.setConfig(config);
  ASSERT_EQ(sm.getAAddress(), 51);
  ASSERT_EQ(sm.getBAddress(), 49);
}

TEST(HNZStateMachine, TestRr) {
  StateMachine sm;
  ASSERT_EQ(sm.getControlRR(0), 1);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 0b100001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 0b1000001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 0b1100001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 0b10000001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 0b10100001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 0b11000001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 0b11100001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlRR(0), 1);
  ASSERT_EQ(sm.getControlRR(1), 0b10001);
}

TEST(HNZStateMachine, TestRej) {
  StateMachine sm;
  ASSERT_EQ(sm.getControlREJ(0), 0b001001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b101001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b1001001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b1101001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b10001001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b10101001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b11001001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b11101001);
  sm.receiveInformation();
  ASSERT_EQ(sm.getControlREJ(0), 0b001001);
  ASSERT_EQ(sm.getControlREJ(1), 0b011001);
}

TEST(HNZStateMachine, TestSendCg) {
  StateMachine sm;
  ASSERT_EQ(sm.isCGSent(),false);
  ASSERT_NO_THROW(sm.setCgSent(true));
  ASSERT_EQ(sm.isCGSent(),true);
  ASSERT_NO_THROW(sm.setCgSent(false));
  ASSERT_EQ(sm.isCGSent(),false);
}