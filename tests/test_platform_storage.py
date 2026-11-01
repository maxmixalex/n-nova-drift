"""Exercise the desktop backend of the same save-generation file protocol."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which("g++"), "host C++ compiler is required")
class PlatformStorageTest(unittest.TestCase):
    def test_generations_exact_length_recovery_and_path_boundary(self):
        code = r'''
#include "platform.h"
#include <assert.h>
#include <stdio.h>
#include <string>
int main(int argc,char** argv) {
    assert(argc==2);
    nova::platform::configure_host(nullptr,nullptr,0,true,false);
    assert(nova::platform::initialize(argv[1]));
    const uint32_t first=0x11223344,second=0x55667788,third=0xaabbccdd;
    uint32_t readback=0;
    assert(nova::platform::write_state("sample.tns",&first,4));
    assert(nova::platform::write_state("sample.tns",&second,4));
    assert(nova::platform::read_state("sample.tns",&readback,4)&&readback==second);
    std::string location=std::string(nova::platform::base_path())+"/state/sample.tns";
    assert(remove(location.c_str())==0);
    assert(nova::platform::read_state("sample.tns",&readback,4)&&readback==first);
    assert(nova::platform::write_state("sample.tns",&third,4));
    assert(nova::platform::read_state("sample.tns",&readback,4)&&readback==third);
    FILE* file=fopen(location.c_str(),"wb");assert(file);fputc(1,file);fclose(file);
    assert(nova::platform::read_state("sample.tns",&readback,4)&&readback==first);
    assert(!nova::platform::open_data("../sample.tns"));
    assert(!nova::platform::write_state("../sample.tns",&first,4));
    nova::platform::shutdown();
}
'''
        with tempfile.TemporaryDirectory(prefix="nnova-storage-") as temporary:
            path=Path(temporary);source=path/"storage.cpp";executable=path/"storage"
            source.write_text(code)
            subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror",
                            "-I"+str(ROOT/"src"),str(ROOT/"src/platform.cpp"),str(source),"-o",str(executable)],check=True)
            subprocess.run([str(executable),str(path/"nNova.tns")],check=True)

if __name__=="__main__": unittest.main()
