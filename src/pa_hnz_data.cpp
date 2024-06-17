#include "pa_hnz_data.h"
#include <bitset>
#include "hnzutility.h"
#include "hnz_server.h"


std::array<unsigned char, TSCG_SIZE> PaHnzData::m_ts_data{};
std::recursive_mutex PaHnzData::m_data_mutex{};

void PaHnzData::setTsValue(unsigned char addr, bool valid, bool open) {
    std::lock_guard<std::recursive_mutex> guard(PaHnzData::m_data_mutex);
    int index = addr / 4;
    int octedIndex = addr % 4 * 2;

    if(addr > TSCG_SIZE - 1 ) {
        return;
    }
   
   auto ts = PaHnzData::m_ts_data[index];
    std::bitset<8> bit(ts);
    if(valid) {
        bit.set(7-octedIndex);
    } else {
        bit.reset(7-octedIndex);
    }
    
    if(open) {
        bit.set(7-(octedIndex+1));
    } else {
        bit.reset(7-(octedIndex+1));
    }
    
    PaHnzData::m_ts_data[index] = static_cast<unsigned char>(bit.to_ulong());
} 


std::vector<unsigned char> PaHnzData::tsValueToTSCG() const {
    std::lock_guard<std::recursive_mutex> guard(PaHnzData::m_data_mutex);
    std::vector<unsigned char> val;
    size_t size = TSCG_SIZE + (TSCG_SIZE / 4 * 2);
    val.reserve(size);
    val.resize(size);
    int index = 0;
    int addr = 0;
    for (int i = 0; i < PaHnzData::m_ts_data.size(); ++i) {
        if(i % 4 == 0) {
            val[index] = TSCG_CODE;
            index++;
            val[index] = static_cast<unsigned char>(addr * 2);
            addr++;
            index++;
        }
        val[index] = PaHnzData::m_ts_data[i];
        
        index++;
    }
    return val;
}