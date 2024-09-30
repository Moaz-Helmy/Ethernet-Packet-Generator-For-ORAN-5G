#ifndef ECPRI_H
#define ECPRI_H

#include <iostream>
#include <fstream>
#include"../ORAN/oran.h"

using namespace std;

#define ECPRI_HEADER_SIZE 8


class ECPRI{
    uint8_t ECPRI_firstByte=0x00;
    uint8_t ecpriMessage=0x00; //User Plane
    uint8_t ecpriPCid_RTCid=0x00;
    uint8_t ecpriSeqId;
    
    public:
    ORAN* oran_obj;
    uint8_t** ECPRI_GeneratedPackets;
    uint64_t numOfECPRIPackets;
    uint64_t ECPRIPacketSize;
    uint8_t ecpriPayload;
    ECPRI(ORAN*);  
    ~ECPRI();  
    void ECPRI_formulatePackets();
    void ECPRI_memoryAllocation();
    void ECPRI_writeGeneratedPacketsToFile();
};


#endif