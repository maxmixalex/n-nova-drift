#pragma once
#include <stdint.h>

namespace nova {
class Locale {
public:
    bool open(bool chinese);
    const char* get(const char* key) const;
    bool ready() const { return ready_; }
    static void plain(char* destination,unsigned capacity,const char* source,bool title=false);
private:
    alignas(4) uint8_t data_[384*1024];
    bool ready_;
};
}
