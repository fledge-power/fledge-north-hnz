#include <gtest/gtest.h>
#include <plugin_api.h>
#include <rapidjson/document.h>
#include <string.h>
#include <config_category.h>

#include <string>

using namespace std;
using namespace rapidjson;

extern "C" {
PLUGIN_INFORMATION *plugin_info();
};

TEST(HNZ, PluginInfo) {
  PLUGIN_INFORMATION *info = nullptr;
  ASSERT_NO_THROW(info = plugin_info());
  ASSERT_NE(info, nullptr);
  ASSERT_STREQ(info->name, "hnz");
  ASSERT_EQ(info->options, SP_CONTROL);
  ASSERT_EQ(info->type, PLUGIN_TYPE_NORTH);
  ASSERT_STREQ(info->interface, "1.0.0");
}

TEST(HNZ, PluginInfoConfigParse) {
  PLUGIN_INFORMATION *info = nullptr;
  ASSERT_NO_THROW(info = plugin_info());
  ASSERT_NE(info, nullptr);
  Document doc;
  doc.Parse(info->config);
  ASSERT_EQ(doc.HasParseError(), false);
  ASSERT_EQ(doc.IsObject(), true);
  ASSERT_EQ(doc.HasMember("plugin"), true);
  ASSERT_EQ(doc.HasMember("protocol_stack"), true);
  ASSERT_EQ(doc["protocol_stack"].IsObject(), true);
  ASSERT_EQ(doc["protocol_stack"].HasMember("default"), true);


  Document stack_doc;
  stack_doc.Parse(doc["protocol_stack"]["default"].GetString());
  ASSERT_EQ(stack_doc.HasParseError(), false);
  ASSERT_EQ(stack_doc["protocol_stack"].IsObject(), true);
  ASSERT_EQ(stack_doc["protocol_stack"].HasMember("transport_layer"), true);
  ASSERT_EQ(stack_doc["protocol_stack"]["transport_layer"].HasMember("port_path_A"), true);
  ASSERT_EQ(stack_doc["protocol_stack"]["transport_layer"].HasMember("port_path_B"), true);
  ASSERT_EQ(stack_doc["protocol_stack"]["transport_layer"]["port_path_A"].IsInt(), true);
  ASSERT_EQ(stack_doc["protocol_stack"]["transport_layer"]["port_path_B"].IsInt(), true);

}

TEST(HNZ, PluginConfigCategory) {
  PLUGIN_INFORMATION *info = nullptr;
  ASSERT_NO_THROW(info = plugin_info());
  ASSERT_NE(info, nullptr);
  const auto config = new ConfigCategory("newConfig", info->config);
  ASSERT_EQ(config->itemExists("protocol_stack"), true);
}