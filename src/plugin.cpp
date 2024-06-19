/*
 * Fledge HNZ north plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Ludovic Le Brun
 */

#include <config_category.h>
#include "hnz.h"
#include <plugin_api.h>
#include <rapidjson/document.h>
#include <version.h>
#include <reading.h>
#include <string>
#include "hnz_config.h"
#include "hnzutility.h"


using namespace std;

#define STRINGIFY(x) #x
#define STRINGIFY_MACRO(x) STRINGIFY(x)

// PLUGIN DEFAULT PROTOCOL STACK CONF
#define PROTOCOL_STACK_DEF                            \
  QUOTE({                                             \
    "protocol_stack" : {                              \
      "name" : "hnzclient",                           \
      "version" : "1.0",                              \
      "transport_layer" : {                           \
          "port_path_A":6001,                         \
          "port_path_B":6002                          \
      },                                              \
      "application_layer":{                           \
         "remote_station_addr":12,                    \
         "inacc_timeout":180,                         \
         "max_sarm":30,                               \
         "repeat_path_A":3,                           \
         "repeat_path_B":3,                           \
         "repeat_timeout":3000,                       \
         "anticipation_ratio":3,                      \
         "test_msg_send":"1304",                      \
         "test_msg_receive":"1304",                   \
         "gi_schedule":"99:99",                       \
         "gi_repeat_count":3,                         \
         "gi_time":255,                               \
         "c_ack_time":10,                             \
         "cmd_recv_timeout":100000                    \
      }                                               \
  }                                                   \
  })

// PLUGIN DEFAULT EXCHANGED DATA CONF
#define EXCHANGED_DATA_DEF                                                     \
  QUOTE({                                                                      \
    "exchanged_data" : {                                                       \
      "name" : "SAMPLE",                                                       \
      "version" : "1.0",                                                       \
      "datapoints" : [                                                         \
        {                                                                      \
          "label" : "TS1",                                                     \
          "pivot_id" : "ID114562",                                             \
          "pivot_type" : "SpsTyp",                                             \
          "protocols" : [                                                      \
            {"name" : "iec104", "address" : "45-672", "typeid" : "M_SP_TB_1"}, \
            {                                                                  \
              "name" : "tase2",                                                \
              "address" : "S_114562",                                          \
              "typeid" : "Data_StateQTimeTagExtended"                          \
            },                                                                 \
            {                                                                  \
              "name" : "hnzip",                                                \
              "address" : "511",                                               \
              "typeid" : "TS"                                                  \
            }                                                                  \
          ]                                                                    \
        },                                                                     \
        {                                                                      \
          "label" : "TM1",                                                     \
          "pivot_id" : "ID99876",                                              \
          "pivot_type" : "DpsTyp",                                             \
          "protocols" : [                                                      \
            {"name" : "iec104", "address" : "45-984", "typeid" : "M_ME_NA_1"}, \
            {                                                                  \
              "name" : "tase2",                                                \
              "address" : "S_114562",                                          \
              "typeid" : "Data_RealQ"                                          \
            },                                                                 \
            {                                                                  \
              "name" : "hnzip",                                                \
              "address" : "511",                                               \
              "typeid" : "TM"                                                  \
            }                                                                  \
          ]                                                                    \
        }                                                                      \
      ]                                                                        \
    }                                                                          \
  })

/**
 * Default configuration
 */

static const char * default_config = QUOTE({
    "plugin": {
      "description" : "hnz north plugin",
      "type" : "string",
      "default" : "hnz",
      "readonly": "true"
    },
    "protocol_stack" : {
      "description" : "protocol stack parameters",
      "type" : "JSON",
      "displayName" : "Protocol stack parameters",
      "order" : "2",
      "default" : PROTOCOL_STACK_DEF,
      "mandatory" : "true"
    },
    "exchanged_data" : {
    "description" : "exchanged data list",
    "type" : "JSON",
    "displayName" : "Exchanged data list",
    "order" : "3",
    "default" : EXCHANGED_DATA_DEF,
    "mandatory" : "true"
  }
});

/**
 * The HNZ plugin interface
 */
extern "C" {
static PLUGIN_INFORMATION info = {
    PLUGIN_NAME,           // Name
    VERSION,               // Version
    SP_CONTROL,            // Flags
    PLUGIN_TYPE_NORTH,     // Type
    "1.0.0",               // Interface version
    default_config         // Default configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info() {
  return &info;
}

PLUGIN_HANDLE plugin_init(ConfigCategory* config) { 

  #ifdef START_LOG_LEVEL
  Logger::getLogger()->setMinLevel(STRINGIFY_MACRO(START_LOG_LEVEL));
  #endif

  std::string beforeLog = HnzUtility::PluginName + " - plugin_init -";
  HnzUtility::log_info("%s Initializing the plugin", beforeLog.c_str());

  if (config == nullptr) {
      HnzUtility::log_warn("%s No config provided for filter, using default config", beforeLog.c_str());
      auto pluginInfo = plugin_info();
      config = new ConfigCategory("newConfig", pluginInfo->config);
      config->setItemsValueFromDefault();
  }

   auto hnz = new HNZ();
   hnz->reconfigure(*config);

  HnzUtility::log_info("%s Plugin initialized", beforeLog.c_str());

  return (PLUGIN_HANDLE)hnz;
}

/**
 * Start the Async handling for the plugin
 */
void plugin_start(PLUGIN_HANDLE *handle) {
  const auto beforeLog = HnzUtility::PluginName + " - plugin_start -";
  HnzUtility::log_debug("%s Starting the plugin...", beforeLog.c_str());
  auto hnz = (HNZ*)handle;
  hnz->start();
  HnzUtility::log_debug("%s Plugin started", beforeLog.c_str());
}


/**
 * Shutdown the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE handle) {
  const auto beforeLog = HnzUtility::PluginName + " - plugin_shutdown -";
  HnzUtility::log_debug("%s Shutting down the plugin...", beforeLog.c_str());
  auto hnz = (HNZ*)handle;
  hnz->stop();
  delete hnz;
}

/**
 * Reconfigure the plugin
 */
void plugin_reconfigure(PLUGIN_HANDLE handle, string &newConfig) {
  if (!handle) throw exception();
  std::string beforeLog = HnzUtility::PluginName + " - plugin_reconfigure -";
  HnzUtility::log_debug("%s New config: %s", beforeLog.c_str(), newConfig.c_str());

  auto hnz = (HNZ *)(handle);

  ConfigCategory config("newConfig", newConfig);

  hnz->reconfigure(config);
}

/**
 * Register the plugin
 */
void plugin_register(PLUGIN_HANDLE handle,
		bool ( *write)(const char *name, const char *value, ControlDestination destination, ...),
		int (* operation)(char *operation, int paramCount, char *names[], char *parameters[], ControlDestination destination, ...))
{
    std::string beforeLog = HnzUtility::PluginName + " - plugin_register -";
    HnzUtility::log_debug("%s Received new write and operation callbacks to regiter", beforeLog.c_str());
    auto hnz = (HNZ*)handle;
    hnz->registerControl(operation);
}

/**
 * Plugin_send entry point
 */
uint32_t plugin_send(PLUGIN_HANDLE handle, std::vector<Reading *>& readings) {
  auto hnz = (HNZ*)handle;
  return hnz->send(readings);
}


}
