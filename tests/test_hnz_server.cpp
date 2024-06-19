#include <gtest/gtest.h>
#include "hnz.h"
#include <config_category.h>
#include <reading.h>
#include <regex>
#include <vector>
#include "hnzutility.h"
static const char *default_config = QUOTE(
    {
        "plugin" : {
            "description" : "hnz north plugin",
            "type" : "string",
            "default" : "hnz",
            "readonly" : "true"
        },
        "protocol_stack" : {
            "description" : "protocol stack parameters",
            "type" : "JSON",
            "displayName" : "Protocol stack parameters",
            "order" : "1",
            "default" : "{ \"protocol_stack\" : { \"name\" : \"hnzclient\", \"version\" : \"1.0\", \"transport_layer\" : { \"port_path_A\": 9090, \"port_path_B\": 9091 }, \"application_layer\" : { \"repeat_timeout\" : 3000, \"repeat_path_A\" : 3, \"remote_station_addr\" : 1, \"max_sarm\" : 5, \"gi_time\" : 1, \"gi_repeat_count\" : 2, \"anticipation_ratio\" : 5 }, \"south_monitoring\" : { \"asset\" : \"TEST_STATUS\" } } }",
            "mandatory" : "true"
        },
        "exchanged_data" : {
            "description" : "exchanged data list",
            "type" : "JSON",
            "displayName" : "Exchanged data list",
            "order" : "3",
            "default" : "{ \"exchanged_data\" : { \"name\" : \"SAMPLE\", \"version\" : \"1.0\", \"datapoints\" : [ { \"label\" : \"TS1\", \"pivot_id\" : \"ID114562\", \"pivot_type\" : \"SpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-672\", \"typeid\" : \"M_SP_TB_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_StateQTimeTagExtended\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TS\" } ] }, { \"label\" : \"TM1\", \"pivot_id\" : \"ID99876\", \"pivot_type\" : \"DpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-984\", \"typeid\" : \"M_ME_NA_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_RealQ\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TM\" } ] } ] } }",
            "mandatory" : "true"
        }
    });

static const char *new_config = QUOTE(
    {
        "plugin" : {
            "description" : "hnz north plugin",
            "type" : "string",
            "default" : "hnz",
            "readonly" : "true"
        },
        "protocol_stack" : {
            "description" : "protocol stack parameters",
            "type" : "JSON",
            "displayName" : "Protocol stack parameters",
            "order" : "1",
            "default" : "{ \"protocol_stack\" : { \"name\" : \"hnzclient\", \"version\" : \"1.0\", \"transport_layer\" : { \"port_path_A\": 6002, \"port_path_B\": 6003 }, \"application_layer\" : { \"repeat_timeout\" : 3000, \"repeat_path_A\" : 3, \"remote_station_addr\" : 1, \"max_sarm\" : 5, \"gi_time\" : 1, \"gi_repeat_count\" : 2, \"anticipation_ratio\" : 5 }, \"south_monitoring\" : { \"asset\" : \"TEST_STATUS\" } } }",
            "mandatory" : "true"
        },
        "exchanged_data" : {
            "description" : "exchanged data list",
            "type" : "JSON",
            "displayName" : "Exchanged data list",
            "order" : "3",
            "default" : "{ \"exchanged_data\" : { \"name\" : \"SAMPLE\", \"version\" : \"1.0\", \"datapoints\" : [ { \"label\" : \"TS1\", \"pivot_id\" : \"ID114562\", \"pivot_type\" : \"SpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-672\", \"typeid\" : \"M_SP_TB_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_StateQTimeTagExtended\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TS\" } ] }, { \"label\" : \"TM1\", \"pivot_id\" : \"ID99876\", \"pivot_type\" : \"DpsTyp\", \"protocols\" : [ {\"name\" : \"iec104\", \"address\" : \"45-984\", \"typeid\" : \"M_ME_NA_1\"}, { \"name\" : \"tase2\", \"address\" : \"S_114562\", \"typeid\" : \"Data_RealQ\" }, { \"name\" : \"hnzip\", \"address\" : \"511\", \"typeid\" : \"TS\" } ] } ] } }",
            "mandatory" : "true"
        }
    });

/**
 * Create a datapoint.
 * @param name
 * @param value
 */
template <class T>
static Datapoint *m_createDatapoint(const std::string &name, const T value)
{
    DatapointValue dp_value = DatapointValue(value);
    return new Datapoint(name, dp_value);
}

Reading *createReading(const ReadingParameters &params)
{
    std::string beforeLog = HnzUtility::PluginName + " - tst_hnz::create_reading - ";
    bool isTS = (params.msg_code == "TS");
    bool isTSCE = isTS && !params.cg;
    bool isTM = (params.msg_code == "TM");
    std::string debugStr = beforeLog + "Send to fledge " + params.msg_code +
                           " with station address = " + std::to_string(params.station_addr) +
                           ", message address = " + std::to_string(params.msg_address) +
                           ", value = " + std::to_string(params.value) + ", valid = " + std::to_string(params.valid);
    if (isTS)
    {
        debugStr += ", cg= " + std::to_string(params.cg);
    }
    if (isTM)
    {
        debugStr += ", an= " + params.an;
    }
    if (isTM || isTS)
    {
        debugStr += ", outdated= " + std::to_string(params.outdated);
        debugStr += ", qualityUpdate= " + std::to_string(params.qualityUpdate);
    }
    if (isTSCE)
    {
        debugStr += ", ts = " + std::to_string(params.ts) + ", iv = " + std::to_string(params.ts_iv) +
                    ", c = " + std::to_string(params.ts_c) + ", s" + std::to_string(params.ts_s);
    }

    HnzUtility::log_debug(debugStr);

    auto *measure_features = new std::vector<Datapoint *>;
    measure_features->push_back(m_createDatapoint("do_type", params.msg_code));
    measure_features->push_back(m_createDatapoint("do_station", static_cast<long int>(params.station_addr)));
    measure_features->push_back(m_createDatapoint("do_addr", static_cast<long int>(params.msg_address)));
    // Do not send value when creating a quality update reading
    if (!params.qualityUpdate)
    {
        measure_features->push_back(m_createDatapoint("do_value", params.value));
    }
    measure_features->push_back(m_createDatapoint("do_valid", static_cast<long int>(params.valid)));

    if (isTM)
    {
        measure_features->push_back(m_createDatapoint("do_an", params.an));
    }
    if (isTS)
    {
        // Casting "bool" to "long int" result in true => 1 / false => 0
        measure_features->push_back(m_createDatapoint("do_cg", static_cast<long int>(params.cg)));
    }
    if (isTM || isTS)
    {
        // Casting "bool" to "long int" result in true => 1 / false => 0
        measure_features->push_back(m_createDatapoint("do_outdated", static_cast<long int>(params.outdated)));
    }
    if (isTSCE)
    {
        // Casting "unsigned long" into "long" for do_ts in order to match implementation of iec104 plugin
        measure_features->push_back(m_createDatapoint("do_ts", static_cast<long int>(params.ts)));
        measure_features->push_back(m_createDatapoint("do_ts_iv", static_cast<long int>(params.ts_iv)));
        measure_features->push_back(m_createDatapoint("do_ts_c", static_cast<long int>(params.ts_c)));
        measure_features->push_back(m_createDatapoint("do_ts_s", static_cast<long int>(params.ts_s)));
    }

    DatapointValue dpv(measure_features, true);

    Datapoint *dp = new Datapoint("data_object", dpv);

    return new Reading(params.label, dp);
}

Reading * jsonToReading(const std::string &jsonStr)
{
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    ReadingParameters readingParams;

    if (doc.HasMember("data_object") && doc["data_object"].IsObject())
    {
        const rapidjson::Value &dataObj = doc["data_object"];
        if (dataObj.HasMember("do_type") && dataObj["do_type"].IsString())
            readingParams.label = dataObj["do_type"].GetString();
            readingParams.msg_code = "TS" ;
        if (dataObj.HasMember("do_station") && dataObj["do_station"].IsUint())
            readingParams.station_addr = dataObj["do_station"].GetUint();
        if (dataObj.HasMember("do_addr") && dataObj["do_addr"].IsUint())
            readingParams.msg_address = dataObj["do_addr"].GetUint();
        if (dataObj.HasMember("do_value") && dataObj["do_value"].IsInt())
            readingParams.value = dataObj["do_value"].GetInt();
        if (dataObj.HasMember("do_valid") && dataObj["do_valid"].IsUint())
            readingParams.valid = dataObj["do_valid"].GetUint();
        if (dataObj.HasMember("do_ts") && dataObj["do_ts"].IsUint64())
            readingParams.ts = dataObj["do_ts"].GetUint64();
        if (dataObj.HasMember("do_ts_iv") && dataObj["do_ts_iv"].IsUint())
            readingParams.ts_iv = dataObj["do_ts_iv"].GetUint();
        if (dataObj.HasMember("do_ts_c") && dataObj["do_ts_c"].IsUint())
            readingParams.ts_c = dataObj["do_ts_c"].GetUint();
        if (dataObj.HasMember("do_ts_s") && dataObj["do_ts_s"].IsUint())
            readingParams.ts_s = dataObj["do_ts_s"].GetUint();
        if (dataObj.HasMember("do_cg") && dataObj["do_cg"].IsBool())
            readingParams.cg = dataObj["do_cg"].GetBool();
        if (dataObj.HasMember("do_an") && dataObj["do_an"].IsString())
            readingParams.an = dataObj["do_an"].GetString();
        if (dataObj.HasMember("do_outdated") && dataObj["do_outdated"].IsBool())
            readingParams.outdated = dataObj["do_outdated"].GetBool();
        // if (dataObj.HasMember("qualityUpdate") && dataObj["qualityUpdate"].IsBool())
        //     readingParams.qualityUpdate = dataObj["qualityUpdate"].GetBool();
    }

    return createReading(readingParams);

}

static std::string readingToJson(const Reading &reading)
{
    std::vector<Datapoint *> dataPoints = reading.getReadingData();
    std::string out = "[";
    static const std::string readingTemplate = QUOTE({"<name>" : <reading>});
    for (Datapoint *sdp : dataPoints)
    {
        std::string reading = std::regex_replace(readingTemplate, std::regex("<name>"), sdp->getName());
        reading = std::regex_replace(reading, std::regex("<reading>"), sdp->getData().toString());
        if (out.size() > 1)
        {
            out += ", ";
        }
        out += reading;
    }
    out += "]";
    return out;
}

class HnzTestServer : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // initialisation des ressources pour les tests
    }

    void TearDown() override
    {
        // nettoyage des ressources après les tests
    }
};

TEST(HnzTestServer, Test1)
{
    HNZ h;
    ConfigCategory config("Test_Config", default_config);
    ConfigCategory config2("NewConfig", new_config);
    config.setItemsValueFromDefault();
    config2.setItemsValueFromDefault();
    h.reconfigure(config);


    std::string jsonStr = R"(
    {
        "data_object":{
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
    }
    )";

    auto reading = jsonToReading(jsonStr);

    std::vector<Reading *> vec;
    vec.push_back(reading);
    h.send(vec);
    h.stop();
}
