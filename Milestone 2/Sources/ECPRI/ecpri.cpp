
#include"ecpri.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;


ECPRI::ECPRI(ORAN* oran_obj=NULL)
{
    if(oran_obj==NULL)
    {
        cout<<"You must provide ORAN object pointer to the ECPRI object.";
        exit(1);
    }
    this->oran_obj=oran_obj;
    ecpriPayload = this->oran_obj->ORANPacketSize;
    numOfECPRIPackets = this->oran_obj->numOfNeededEthernetPackets;
    ECPRIPacketSize = ECPRI_HEADER_SIZE + ecpriPayload;
    ECPRI_memoryAllocation();
    ECPRI_formulatePackets();
}

ECPRI::~ECPRI()
{
    for(int packet=0 ; packet<numOfECPRIPackets;packet++)
    {
        delete[] ECPRI_GeneratedPackets[packet];
    }
    delete[] ECPRI_GeneratedPackets;
}

void ECPRI::ECPRI_memoryAllocation()
{
    ECPRI_GeneratedPackets = new(nothrow) uint8_t*[numOfECPRIPackets];
    if(ECPRI_GeneratedPackets==nullptr)
    {
        cout<<"Memory Failure.";
        exit(1);
    }
    for(int pacekt=0;pacekt<numOfECPRIPackets; pacekt++)
    {
        
        ECPRI_GeneratedPackets[pacekt] = new (nothrow) uint8_t[ECPRIPacketSize];
        if(ECPRI_GeneratedPackets[pacekt]==nullptr)
        {
            cout<<"Memory Failure.";
            exit(1);
        }
    }
}

void ECPRI::ECPRI_formulatePackets()
{
    int ecpriByte=0;
    ecpriSeqId=0;

    for (int packet=0 ;packet<numOfECPRIPackets;packet++)
    {
        ECPRI_GeneratedPackets[packet][0]=ECPRI_firstByte;
        ECPRI_GeneratedPackets[packet][1]=ecpriMessage;
        ECPRI_GeneratedPackets[packet][2]=ecpriPayload;
        ECPRI_GeneratedPackets[packet][3]=ecpriPCid_RTCid;
        ECPRI_GeneratedPackets[packet][4]=ecpriSeqId;
        ecpriSeqId++;
        ecpriByte=5;
        for(int byte=0;byte<ecpriPayload;byte++)
        {
            ECPRI_GeneratedPackets[packet][ecpriByte]=oran_obj->ORAN_GeneratedPackets[packet][byte];
            ecpriByte++;
        }
    }
    ECPRI_writeGeneratedPacketsToFile();
}

void ECPRI::ECPRI_writeGeneratedPacketsToFile()
{
    ofstream outFile("ECPRI Debug Files/Generated_ECPRI_Packets.txt", ios::out);
    if (!outFile.is_open())
    {
        cout << "Couldn't Open File.";
        exit(1);
    }
    for (int packet = 0; packet < numOfECPRIPackets; packet++)
    {
        for (int byte = 0; byte < ECPRIPacketSize; byte++)
        {
            outFile << hex
                    << uppercase
                    << setw(2)
                    << setfill('0')
                    << static_cast<int>(ECPRI_GeneratedPackets[packet][byte]) << " ";
        }
        outFile << endl;
    }

    outFile.close();
}
