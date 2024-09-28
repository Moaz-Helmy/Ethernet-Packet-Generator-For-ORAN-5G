
#include"ecpri.h"
#include <iostream>
#include <fstream>

using namespace std;


ECPRI::ECPRI(ORAN* oran_obj=NULL)
{
    if(oran_obj==NULL)
    {
        cout<<"You must provide ORAN object pointer to the ECPRI object.";
        exit(1);
    }

    ecpriPayload = oran_obj->ORANPacketSize;
    numOfECPRIPackets = oran_obj->numOfNeededEthernetPackets;
    ECPRIPacketSize = ECPRI_HEADER_SIZE + ecpriPayload;

    ECPRI_memoryAllocation();
}

ECPRI::~ECPRI()
{
    
}

void ECPRI::ECPRI_memoryAllocation()
{
    
}
