#pragma once 

#include <array>
#include <vector>
#include <mutex>

const int TSCG_SIZE = 9*4;

/**
 * @class PaHnzData
 * @brief Manages and processes HNZ protocol data.
 *
 * The PaHnzData class is responsible for handling HNZ protocol data,
 * including setting values and converting them to a specific format.
 */
class PaHnzData {

public:
    /**
     * @brief Sets the value for a given address.
     *
     * @param addr The address for which the value is being set.
     * @param valid A boolean indicating whether the value is valid.
     * @param open A boolean indicating whether the value represents an open state.
     *
     * This method sets the value for a specific TS address in the HNZ protocol data.
     */
    static void setTsValue(unsigned char addr, bool valid, bool open);

    /**
     * @brief Converts the current values to the TSCG format.
     *
     * @return A vector of unsigned characters representing the values in TSCG format.
     *
     * This method converts the current HNZ protocol data values to a format
     * specified by TSCG and returns them as a vector of unsigned characters.
     */
    std::vector<unsigned char> tsValueToTSCG() const;
private:
    static std::array<unsigned char, TSCG_SIZE> m_ts_data;
    static std::recursive_mutex m_data_mutex;
};