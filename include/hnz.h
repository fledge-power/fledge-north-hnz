#pragma once

#include <vector>
#include <string>
#include <array>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <plugin_api.h>
#include "hnz_config.h"
#include "enhanced_hnz_server.h"
#include <datapoint.h>

// forward declaration
class Reading;
class ConfigCategory;


// Enums put outside of the HNZ class so that they can be forward declared
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

// Dedicated structure used to store parameters passed to m_prepare_reading.
// This prevents "too many parameters" warning from Sonarqube (cpp:S107).
struct ReadingParameters
{
  // Those are mandatory parameters
  std::string label;
  std::string msg_code;
  unsigned int station_addr = 0;
  unsigned int msg_address = 0;
  long int value = 0;
  unsigned int valid = 0;
  // Those are optional parameters
  // TSCE only
  unsigned long ts = 0;
  unsigned int ts_iv = 0;
  unsigned int ts_c = 0;
  unsigned int ts_s = 0;
  // TS only
  bool cg = false;
  // TM only
  std::string an = "";
  // TS and TN only
  bool outdated = false;
  bool qualityUpdate = false;
};

/**
 * @brief Class used to receive messages and push them to fledge.
 */
class HNZ
{
public:

  /**
   * Start the HZN south plugin
   * @param requestedStart tells if start was requested by the Fledge API
   */
  void start();

  /**
   * Stop the HZN south plugin
   * @param requestedStop tells if stop was requested by the Fledge API
   */
  void stop();

  /**
   * Thread used for manage tcp server
   * @param server hnz server
   */
  void managerServer(const std::shared_ptr<EnhancedHNZServer> &server);

  /**
   * Thread used for the hnz servers
   */
  void serverHnzThread();

  /**
   * @brief Registers a control operation to be invoked later.
   *
   * This method is called by a North plugin in Fledge Power to register an operation.
   *
   * @param operation A function pointer to the operation to be registered.
   * @param paramCount An integer specifying the number of parameters expected by the operation function.
   * @param names An array of strings containing the names of the parameters.
   * @param parameters An array of strings containing the values of the parameters.
   * @param destination An enum specifying the destination of the control command.
   * @param ... Additional arguments required for specifying the destination, if necessary.
   *
   * @return void
   */
  void registerControl(int (*operation)(char *operation, int paramCount, char *names[], char *parameters[], ControlDestination destination, ...));

  /**
   * Send the set of readings to the north system.
   *
   * Uses the priamry or secondary connection, if defined to send
   * the readings as JSON to the north FogLAMP.
   *
   * The member varaible m_failedOver is used to determine if the
   * primary or secoindary connection should be used.
   *
   * @param readings	The readings to send
   * @return uint32_t 	The numebr of readings sent
   */
  uint32_t send(std::vector<Reading *> &readings);

  /**
   * @brief Reconfigures the server with the provided configuration.
   *
   * This method reconfigures the server with the settings specified in the
   * provided configuration ConfigCategory.
   *
   * @param config a ConfigCategory containing the new configuration.
   */
  void reconfigure(const ConfigCategory &config);

  /**
   * @brief Allocates and returns a two-dimensional array of characters (strings).
   *
   * This function allocates memory for an array designed to store `array_size` strings,
   * each with a maximum length of `string_size` characters, including the null terminator.
   *
   * @param array_size The number of strings in the array.
   * @param string_size The maximum length of each string, including the null terminator.
   * @return A pointer to the first element of the allocated array of character pointers.
   * Each pointer in the array points to a character array of size `string_size`.
   */
  char ** getCommandValues(size_t array_size, size_t string_size);


  /**
   * @brief Frees the memory allocated for a two-dimensional array of characters (strings).
   *
   * This function should be used to clean up the memory allocated by `getCommandValues`.
   *
   * @param command A pointer to the first element of the array to be freed.
   * @param size The number of strings in the array.
   */
  void cleanCommandValues(char **command, size_t size);

  /**
   * @brief Sets the value of a string in the two-dimensional character array.
   *
   * The value is copied from a given `std::string` and truncated if necessary to fit within
   * the specified maximum size.
   *
   * @param command A pointer to the character array where the value will be set.
   * @param value The `std::string` containing the value to set.
   * @param max_size The maximum size of the character array, including the null terminator.
   * The function ensures that the string is properly null-terminated.
   */
  void setCommandValue(char *command, const std::string & value, size_t max_size) const;

  /**
   * @brief Processes a Reading object.
   *
   * This function takes a pointer to a Reading object and performs specific
   * operations or processing on it. (HNZ object)
   *
   * @param reading A pointer to a Reading object that will be processed.
   **/
  bool readDataObject(Reading*  reading);

  /**
   * @brief Sets the reading parameters using the provided datapoint.
   * 
   * This method configures the provided ReadingParameters object with data from
   * the specified Datapoint object.
   * 
   * @param params A reference to a ReadingParameters object that will be updated.
   * @param dp A constant pointer to a Datapoint object providing the necessary data.
   * 
   * @note Ensure the dp pointer is not null before calling this method to avoid
   * dereferencing a null pointer.
   * 
   */
  void setReadingParameters(ReadingParameters &params, Datapoint *const dp) const;

  /**
   * @brief Manages the received frames with server context.
   * 
   * This method processes a collection of received frames, each represented by a
   * shared pointer to an MSG_TRAME object, using the provided EnhancedHNZServer context.
   * 
   * @param receivedFrames A constant reference to a vector of shared pointers to
   * MSG_TRAME objects. This vector contains the frames that need to be managed.
   * @param server A shared pointer to an EnhancedHNZServer object providing the server context.
   * 
   * @note This method does not modify the receivedFrames vector or the MSG_TRAME objects.
   * 
   */
  void manageReceivedFrames(const std::vector<std::shared_ptr<MSG_TRAME>> & receivedFrames,const std::shared_ptr<EnhancedHNZServer> &server);

  /**
   * @brief Sends waiting data using the provided server.
   * 
   * This method sends any data that is waiting to be sent using the specified EnhancedHNZServer.
   * 
   * @param server A shared pointer to an EnhancedHNZServer object used to send the waiting data.
   * 
   */
  void sendWaitingData(const std::shared_ptr<EnhancedHNZServer> &server);
private:

  /**
   * Parses TSCE (Time-Series with Context Encoding) data according to the given parameters.
   * @param params The parameters required for parsing the TSCE data.
   */
  void m_parseTSCE(const ReadingParameters &params);

  /**
   * Parses ATVC (Advanced Time-Varying Context) data based on the provided parameters.
   * @param params The parameters necessary for parsing the ATVC data.
   */
  void m_parseATVC(const ReadingParameters &params);

  /**
   * Parses TVC (Time-Varying Context) data using the specified parameters.
   * @param params The parameters needed for parsing the TVC data.
   */
  void m_parseTVC(const ReadingParameters &params);

  /**
   * Parses TM  data according to the given parameters.
   * @param params The parameters required for parsing the TM data.
   */
  void m_parseTM(const ReadingParameters &params);

  /**
   * Performs a specific operation with parameters.
   * @param operation A string representing the operation to be performed.
   * @param paramCount The number of parameters provided.
   * @param names An array of parameter names.
   * @param parameters An array of parameter values corresponding to the names array.
   * @return An integer representing the result of the operation. The meaning of the return value depends on the operation.
   */
  int operation(char *operation, int paramCount, char *names[], char *parameters[]);

  std::shared_ptr<HnzConfig> m_hnz_config;

  std::vector<std::vector<unsigned char>> m_hnz_data_to_send{};

  std::mutex m_data_mtx{};

  std::mutex m_read_mtx{};

  // Tells if the plugin is currently running
  bool m_is_running{false};

  // Hnz server thread
  std::thread m_hnz_thread{};

  // function pointer for operation callback
  int (*m_oper)(char *operation, int paramCount, char *names[], char *parameters[], ControlDestination destination, ...){nullptr};
};