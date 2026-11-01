#include "packed_file.h"
#include "nnova/blob_codec.h"
namespace nova {
namespace { alignas(4) uint8_t packed_scratch[1024*1024]; }
bool read_packed(platform::File* file,uint8_t* target,unsigned capacity,uint32_t& size) {
    size=0;if(!file)return false;
    uint32_t bytes=platform::length(file);
    if(bytes>sizeof(packed_scratch)||bytes<8){platform::close(file);return false;}
    bool ok=platform::read(file,packed_scratch,bytes)==bytes;platform::close(file);
    if(!ok)return false;
    if(blobcodec::is_compressed(packed_scratch,bytes)) {
        size_t decoded=0;
        if(!blobcodec::decode(packed_scratch,bytes,target,capacity,decoded))return false;
        size=uint32_t(decoded);return true;
    }
    if(bytes>capacity)return false;
    for(unsigned i=0;i<bytes;++i)target[i]=packed_scratch[i];
    size=bytes;return true;
}
}
