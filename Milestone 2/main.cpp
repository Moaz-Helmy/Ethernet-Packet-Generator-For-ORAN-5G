#include <iostream>
#include "Sources/Parser/parser.h"
#include "Sources/Ethernet/ethernet.h"
#include "Sources/ORAN/oran.h"
#include "Sources/ECPRI/ecpri.h"
using namespace std;


int main(int argc, char *argv[])
{
    EthConfig Eth;
    ORANConfig Oran;
    
    Parser parse_obj(argv[1], &Eth, &Oran);
    ORAN oran_obj(&Oran);
    ECPRI ecpri_obj(&oran_obj);
    EthPacket obj(&Eth,&ecpri_obj, argv[2]);
    
    
    return 0;
}