#include <gtest/gtest.h>
#include "server_wrapper.h"
#include "hnz_server.h"
#include <memory>
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

TEST(HNZServerWrapper, TestHNZServerWrapper) {
  ServersWrapper wrapper(6001,6002);

  ASSERT_NE(wrapper.server1().get(), nullptr);
  ASSERT_NE(wrapper.server2().get(), nullptr);
  ASSERT_EQ(wrapper.server1()->getPort(), 6001);
  ASSERT_EQ(wrapper.server2()->getPort(), 6002);
}

TEST(HNZServerWrapper, TestHNZServerCheckReceivingControl) {
  EnhancedHNZServer m_server1(6001);
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_EQ(m_server1.checkReceivingControl(UA_CODE,2),0);

  ASSERT_EQ(m_server1.checkReceivingControl(SARM_CODE,2),0);
  ASSERT_EQ(m_server1.checkReceivingControl(0x43,2),0);
  ASSERT_EQ(m_server1.checkReceivingControl(0x60,2),2);
  
}

TEST(HNZServerWrapper, TestHNZServerCheckInformationFrame) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);
  std::vector<unsigned char> data(6,0);
  ASSERT_NO_THROW(m_server1.checkInformationFrame(0,data.data(), false));

  data[2] = CG_CODE;
  data[3] = 0x1;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(4,data.data(), false));

  data[2] = CG_CODE;
  data[3] = 0x4;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(4,data.data(), false));

  data[2] = CG_CODE;
  data[3] = 0x4;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(4,data.data(), false));

  data[2] = TC_CODE;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(4,data.data(), false));

  data[2] = 0x1D;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(5,data.data(), false));

  data[2] = 0x1C;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(6,data.data(), false));

  data[2] = TVC_CODE;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(3,data.data(), false));

  data[2] = 0;
  ASSERT_NO_THROW(m_server1.checkInformationFrame(3,data.data(), false));

}

TEST(HNZServerWrapper, TestHNZServerAnalyzeReceivingFrame) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_NO_THROW(m_server1.analyzeReceivingFrame(nullptr));
  MSG_TRAME frame;
  frame.usLgBuffer = 0;
  ASSERT_NO_THROW(m_server1.analyzeReceivingFrame(&frame));

  frame.usLgBuffer = 1;
  ASSERT_NO_THROW(m_server1.analyzeReceivingFrame(&frame));

  //RR
  frame.aubTrame[1] = 1;
  ASSERT_NO_THROW(m_server1.analyzeReceivingFrame(&frame));

  //REJ
  frame.aubTrame[1] = 0b1001;
  ASSERT_NO_THROW(m_server1.analyzeReceivingFrame(&frame));

  frame.usLgBuffer = 3;
  frame.aubTrame[1] = 0;
  ASSERT_NO_THROW(m_server1.analyzeReceivingFrame(&frame));

  frame.usLgBuffer = 3;
  frame.aubTrame[1] = 0b00010000;
  ASSERT_NO_THROW(m_server1.analyzeReceivingFrame(&frame));
}

TEST(HNZServerWrapper, TestHNZServerReceiving_loop) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  // Launch the thread
  std::thread t1(&EnhancedHNZServer::receiving_loop,&m_server1);
  t1.join();

  ASSERT_NO_THROW(m_server1.startHNZServer());

  m_server1.ServerIsReady(3);
  // Launch the thread
  std::thread t2(&EnhancedHNZServer::receiving_loop,&m_server1);
  
  ASSERT_NO_THROW(m_server1.stopHNZServer());

  t2.join();
}

TEST(HNZServerWrapper, TestHNZServerToHexStr) {
  EnhancedHNZServer m_server1(6001);
 
  ASSERT_EQ(m_server1.toHexStr(5), "0x05");
  ASSERT_EQ("", "");
}

TEST(HNZServerWrapper, TestHNZServerFrameToStr) {
  EnhancedHNZServer m_server1(6001);
  auto frame = std::make_shared<MSG_TRAME>();
  ASSERT_EQ(m_server1.frameToStr(frame), "\n[]");
  ASSERT_EQ(m_server1.frameToStr(nullptr), "");
  frame->usLgBuffer = 3;
  frame->aubTrame[0] = 5;
  frame->aubTrame[1] = 6;
  frame->aubTrame[2] = 7;
  ASSERT_EQ(m_server1.frameToStr(frame), "\n[0x05, 0x06, 0x07]");
  ASSERT_EQ(m_server1.framesToStr({}), "");
  ASSERT_EQ(m_server1.framesToStr({nullptr,nullptr}), "");
  ASSERT_EQ(m_server1.framesToStr({frame,frame}), "\n[0x05, 0x06, 0x07]\n[0x05, 0x06, 0x07]");
}

TEST(HNZServerWrapper, TestHNZServerSend) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);


  ASSERT_NO_THROW(m_server1.startHNZServer());

  ASSERT_NO_THROW(m_server1.sendSARM());

  ASSERT_EQ(m_server1.popLastFramesSent().size(),1);

  ASSERT_NO_THROW(m_server1.sendUA());

  ASSERT_EQ(m_server1.popLastFramesSent().size(),1);


  ASSERT_NO_THROW(m_server1.sendInformation({1,2,3}, false));

  ASSERT_EQ(m_server1.popLastFramesSent().size(),1);

  ASSERT_NO_THROW(m_server1.sendInformation({1,2,3}, true));

  ASSERT_EQ(m_server1.popLastFramesSent().size(),1);

  ASSERT_NO_THROW(m_server1.sendInformation({}, true));

  ASSERT_EQ(m_server1.popLastFramesSent().size(),1);

  ASSERT_NO_THROW(m_server1.sendRR(false));

  ASSERT_EQ(m_server1.popLastFramesSent().size(),1);

  ASSERT_EQ(m_server1.popLastFramesReceived().size(), 0);
  ASSERT_NO_THROW(m_server1.stopHNZServer());


}

TEST(HNZServerWrapper, TestHNZManageReceivedFrames) {
  auto m_server1 = std::make_shared<EnhancedHNZServer>(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1->getStateMachine()->setConfig(config);


  ASSERT_NO_THROW(m_server1->startHNZServer());

  ASSERT_NO_THROW(m_server1->sendSARM());

  auto frames = m_server1->popLastFramesSent();
  ASSERT_EQ(frames.size(),1);

  auto cgFrame = std::make_shared<MSG_TRAME>();
  cgFrame->aubTrame[2] = CG_CODE;
  cgFrame->aubTrame[3] = 0x1;
  cgFrame->usLgBuffer = 4;
  frames.push_back(cgFrame);

  frames.push_back(nullptr);

  auto smallFrame = std::make_shared<MSG_TRAME>();
  smallFrame->aubTrame[2] = TC_CODE;
  smallFrame->usLgBuffer = 2;
  frames.push_back(smallFrame);

  auto tcFrame = std::make_shared<MSG_TRAME>();
  tcFrame->aubTrame[2] = TC_CODE;
  tcFrame->usLgBuffer = 4;
  frames.push_back(tcFrame);

  auto tvcFrame = std::make_shared<MSG_TRAME>();
  tvcFrame->aubTrame[2] = TVC_CODE;
  tvcFrame->usLgBuffer = 4;
  frames.push_back(tvcFrame);

  HNZ hnz;
  ASSERT_NO_THROW(hnz.manageReceivedFrames(frames,m_server1));

}

TEST(HNZServerWrapper, TestHNZServerServerIsReady) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_NO_THROW(m_server1.startHNZServer());

  ASSERT_NO_THROW(m_server1.waitForTCPConnection(2));

  // Launch the thread
  std::thread t1(&EnhancedHNZServer::ServerIsReady,&m_server1,2);

  ASSERT_NO_THROW(m_server1.stopHNZServer());

  t1.join();

}

TEST(HNZServerWrapper, TestHnzSendSARMLOOP) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_NO_THROW(m_server1.startHNZServer());

  ASSERT_NO_THROW(m_server1.waitForTCPConnection(2));

  // Launch the thread
  std::thread t1(&EnhancedHNZServer::sendSARMLoop,&m_server1);

  ASSERT_NO_THROW(m_server1.stopHNZServer());

  t1.join();

}

TEST(HNZServerWrapper, TestHnzSendSARMLOOPRunning) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_NO_THROW(m_server1.startHNZServer());

  ASSERT_NO_THROW(m_server1.waitForTCPConnection(2));
  ASSERT_NO_THROW(m_server1.setIsRunning(true));
  // Launch the thread
  std::thread t1(&EnhancedHNZServer::sendSARMLoop,&m_server1);

  ASSERT_NO_THROW(m_server1.stopHNZServer());

  t1.join();

}

TEST(HNZServerWrapper, TestHnzHandShake) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_NO_THROW(m_server1.startHNZServer());

  ASSERT_NO_THROW(m_server1.waitForTCPConnection(2));

  // Launch the thread
  std::thread t1(&EnhancedHNZServer::handChecking,&m_server1,15);

  ASSERT_NO_THROW(m_server1.stopHNZServer());

  t1.join();

}

TEST(HNZServerWrapper, TestHnzHandShakeRunning) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_NO_THROW(m_server1.startHNZServer());

  ASSERT_NO_THROW(m_server1.waitForTCPConnection(2));
  ASSERT_NO_THROW(m_server1.setIsRunning(true));

  m_server1.handChecking(2);

  ASSERT_NO_THROW(m_server1.setIsRunning(false));
  ASSERT_NO_THROW(m_server1.stopHNZServer());


}

TEST(HNZServerWrapper, TestServeurIsRunning) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);

  ASSERT_FALSE(m_server1.isRunning());
  ASSERT_NO_THROW(m_server1.setIsRunning(true));
  ASSERT_TRUE(m_server1.isRunning());
}

TEST(HNZServerWrapper, TestReceivingLoop) {
  EnhancedHNZServer m_server1(6001);
 
  auto config = std::make_shared<HnzConfig>(protocol_stack, exchanged_data);
  m_server1.getStateMachine()->setConfig(config);
  ASSERT_NO_THROW(m_server1.startHNZServer());
  ASSERT_NO_THROW(m_server1.setIsRunning(true));
  m_server1.receiving_loop();
  ASSERT_NO_THROW(m_server1.stopHNZServer());

}





