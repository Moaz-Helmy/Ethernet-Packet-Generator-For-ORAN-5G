#ifndef ORAN_H
#define ORAN_H

#include<iostream>
#include<fstream>

using namespace std;

#define MINIMAL_SCS 15
#define SYMBOLS_PER_SLOT_NORMAL_CP  14
#define SYMBOLS_PER_SLOT_EXTENDED_CP  12
#define IQ_SAMPLES_PER_RB   12
#define ORAN_HEADER_SIZE 4
#define RB_HEADER_SIZE 4
#define SAMPLE_BYTES 2
#define BITS_PER_BYTE 8
#define SUBFRAMES_PER_FRAME 10

struct ORANConfig{
    uint8_t SCS;
    uint16_t MaxNrb;
    uint8_t NrbPerPacket;
    string PayloadType;
    string payload;
    uint64_t payloadParameterLine;
    uint64_t numOfSubframes;
};

class ORAN{
    short* I;
    short* Q;
    string* fileData;
    ofstream debugFile;
    ifstream IQFile;
    
    
    string temp;
    uint64_t IQ_sample_index;

    
    uint8_t* ORAN_AllPacketsConcatenated;

    void ORAN_convertToByteArray(uint64_t , uint8_t , uint8_t *);
    public:
    ORANConfig* oran_config;
    uint64_t linesCounter;
    uint8_t** ORAN_GeneratedPackets;

    uint64_t numOfSlotsPerSubframe;
    uint64_t numOfTotalSlots;
    uint64_t numOfSymbolsPerSlot;
    uint64_t numOfTotalSymols;
    uint64_t numOfNeededIQSamples;
    uint64_t numOfNeededEthernetPackets;
    uint64_t ORANPacketHeaderSize;
    uint64_t numOfIQSamplesPerPacket;
    uint64_t ORANPacketSize;
    uint64_t numOfFrames;
    uint64_t numOfSlotsPerFrame;
    uint64_t numOfSymbolsPerFrame;
    uint64_t numOfNrbsPerFrame;
    uint64_t numOfPacketsPerFrame;
    uint64_t numOfPacketsPerSubframe;
    uint64_t numOfPacketsPerSlot;
    uint64_t numOfPacketsPerSymbol;
    uint64_t numOfErrorPacketsPerSubframe;
    uint64_t numOfErrorPacketsPerSlot;
    uint64_t numOfErrorPacketsPerSymbol;
    ORAN(ORANConfig*);
    ~ORAN();
    void ORAN_checkPayloadTypeAndAllocateMemory();
    void ORAN_readIQSamples();
    void ORAN_parametersCalculations();
    void ORAN_memoryAllocation();
    void ORAN_fillPacketsHeader();
    void ORAN_writeGeneratedPacketsToFile();
    void ORAN_adjustHeaderBytes();
    void ORAN_fillPayload();
};

#endif