*****************************************************
Unit Test for HNZ south plugin
*****************************************************

Require Google Unit Test framework
Install with:
::
    sudo apt-get install libgtest-dev
    cd /usr/src/gtest
    cmake CMakeLists.txt
    sudo make
    sudo cp lib/*.a /usr/lib/

To build the HNZ unit tests:
::
    mkdir build
    cd build
    cmake ..
    make

To run the tests :
::
    cd build
    ./RunTests