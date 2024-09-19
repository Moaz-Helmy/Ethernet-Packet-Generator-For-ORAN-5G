#include <iostream>
using namespace std;
  
#include "Sources/Parser/parser.h"
#include "Sources/Ethernet/ethernet.h"

int main(int argc, char *argv[])
{
    EthConfig Eth;
    Parser parse_obj(argv[1], &Eth);
    EthPacket obj(&Eth, argv[2]);
    
    return 0;
}