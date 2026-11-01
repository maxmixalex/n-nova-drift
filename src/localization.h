#pragma once
#include <stdint.h>

namespace nova {
class Locale {
public:
    enum { Count=11 };
    bool open(unsigned language);
    bool openFile(const char* filename);
    const char* get(const char* key) const;
    const char* text(const char* key,const char* fallback="") const;
    bool ready() const { return ready_; }
    unsigned index() const { return language_; }
    static const char* code(unsigned language);
    static const char* name(unsigned language);
    static int fromSystem(const char* tag);
    static void plain(char* destination,unsigned capacity,const char* source,bool title=false);
    // Resolves original-game numeric markup. Returns the unresolved count.
    static unsigned format(char* destination,unsigned capacity,const char* source,bool rich=false);
private:
    alignas(4) uint8_t data_[384*1024];
    bool ready_;
    unsigned language_;
};
}
