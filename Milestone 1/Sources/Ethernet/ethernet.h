/**********File protection*************/
#ifndef ETHERNET_H
#define ETHERNET_H

/*************Constants***************/

#define BITS_PER_BYTE           8
#define PREAMBLE_SFD            0xFB555555555555D5
#define ETHER_TYPE              0x0806
#define ETHER_TYPE_SIZE         2
#define PREAMBLE_SFD_SIZE       8
#define IFG                     0x07
#define ADDRESS_SIZE            6
#define CRC_SIZE                4



#include <fstream>
#include <iostream>
using namespace std;
/*************Declarations***************/
/*
    Description: A structure that holds ethernet fields. It'll be used for configuration purposes
    Attributes:
        - eth_DestAdr --> 6 Bytes
        - eth_SrcAdr --> 6 Bytes
        - eth_length --> 2 Bytes
        - eth_payload --> 46:1500 Bytes
*/

struct EthConfig{
    uint16_t LineRate;
    uint16_t CaptureSizeMs;
    uint16_t MinNumOfIFGsPerPacket;
    uint64_t DestAddress;
    uint64_t SourceAddress;
    uint16_t MaxPacketSize;
    uint16_t BurstSize;
    uint16_t BurstPeriodicity_us;
};
/*
    Description: A class that contains all the ethernet frame information
    Attributes:
        - eth_preamble --> 6 Bytes
        - eth_SFD --> 1 Byte
        - eth_DestAdr --> 6 Bytes
        - eth_SrcAdr --> 6 Bytes
        - eth_length --> 2 Bytes
        - eth_payload --> 46:1500 Bytes
        - eth_CRC --> 4 Bytes
    Methods:

*/

class EthPacket
{
    ofstream outputFileHandler;
    uint64_t eth_packetIndex;
    uint16_t eth_EtherType = ETHER_TYPE;
    uint16_t eth_PacketSize;
    uint8_t *eth_Payload;
    uint8_t *eth_IFGsPerPacket;
    uint32_t eth_CRC;
    uint8_t **eth_GeneratedPackets;
    uint8_t *eth_ExtraBytes;
    uint8_t *eth_IdleBytes;
    EthConfig *eth_p;

    uint64_t generatedBytes;
    uint16_t numOfExtraBytes; // IFGs incase of interrupted packet generation
    uint64_t numOfGeneratedPackets;
    uint8_t numOfExtraIFGs; // for alignment
    uint64_t payloadSize;
    uint16_t numOfPacketsPerPeriod;
    uint16_t numOfIdlePackets;
    uint16_t numOfIdleBytes;
    uint16_t numOfTotalIFGs;
    uint16_t numOfTotalIdleBytesPerPeriod;
    uint16_t numOfPacketsInLastBurst;
    uint16_t numOfCompleteBursts;

    void eth_writeGeneratedPackets(string , uint16_t);

    void eth_randomizePayload();
    void eth_convertToByteArray(uint64_t , uint8_t , uint8_t *);

    uint32_t eth_calculate4ByteCRC(uint8_t *, uint32_t );

    void eth_writeIntoGeneratedPacketsDB(uint8_t *, uint16_t , uint16_t , string );

public:
    EthPacket(EthConfig *, string );

    ~EthPacket();

    void eth_ethernetParameterCalculations();

    void eth_memoryAllocations();

    void eth_fillGeneratedPackets();

    void eth_fillPreambleSFD();

    void eth_fillDestAddress();

    void eth_fillSourceAddress();

    void eth_fillEtherType();

    void eth_fillPayload();

    void eth_fillCRC();
    void eth_fillIFGs();

    void eth_generateExtraBytesForInterruptedPacket();

    void eth_generateIdleBytes();

    void eth_sendPacket(uint8_t *);
    void eth_sendIdleBytes();

    void eth_sendExtraBytes();

    void eth_sendAll();

    void eth_sendCompleteBursts();

    
    void eth_sendLastBurtAndExtraBytes();
};
#endif //ETHERNET_H