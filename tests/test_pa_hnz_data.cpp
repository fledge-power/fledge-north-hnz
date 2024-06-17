#include <gtest/gtest.h>
#include "pa_hnz_data.h"


TEST(HNZData, TestPaHnzDataBadAddrr) {
  PaHnzData data;
  const auto tscg = data.tsValueToTSCG();
  data.setTsValue(250, true, true);
  ASSERT_EQ(tscg[2], tscg[2]);
}

TEST(HNZData, TestPaHnzDataFirstAddr) {
  PaHnzData data;
  data.setTsValue(0, true, true);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_TRUE(tscg[2] & 0b11000000);
}

TEST(HNZData, TestPaHnzDataSecondAddr) {
  PaHnzData data;
  data.setTsValue(1, true, true);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_TRUE(tscg[2] & 0b00110000);
}

TEST(HNZData, TestPaHnzDataThirdAddr) {
  PaHnzData data;
  data.setTsValue(2, true, true);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_TRUE(tscg[2] & 0b00001100);
}

TEST(HNZData, TestPaHnzDataForthAddr) {
  PaHnzData data;
  data.setTsValue(3, true, true);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_TRUE(tscg[2] & 0b00000011);
}

TEST(HNZData, TestPaHnzDataFifthAddr) {
  PaHnzData data;
  data.setTsValue(4, true, true);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_TRUE(tscg[3] & 0b11000000);
}

TEST(HNZData, TestPaHnzDataFirstAddrFalse) {
  PaHnzData data;
  data.setTsValue(0, false, false);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_FALSE(tscg[2] & 0b00000000);
}

TEST(HNZData, TestPaHnzDataFirstAddrTrueFalse) {
  PaHnzData data;
  data.setTsValue(0, true, false);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_TRUE(tscg[2] & 0b10000000);
}

TEST(HNZData, TestPaHnzDataFirstAddrFalseTrue) {
  PaHnzData data;
  data.setTsValue(0, false, true);
  const auto tscg = data.tsValueToTSCG();
  ASSERT_TRUE(tscg[2] & 0b01000000);
}