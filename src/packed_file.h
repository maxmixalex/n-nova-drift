#pragma once
#include "platform.h"
namespace nova {
// Shared transient buffer keeps locale/font loading bounded; decoding happens
// only on startup or language changes. Supports legacy uncompressed packs.
bool read_packed(platform::File* file,uint8_t* target,unsigned capacity,uint32_t& size);
}
