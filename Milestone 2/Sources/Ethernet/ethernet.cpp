#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <new>
#include <assert.h>
#include <iostream>
#include "ethernet.h"
#include <string>
using namespace std;

#define DEBUG_MODE_ON

void EthPacket::eth_writeGeneratedPackets(string fileName, uint16_t size)
{
    ofstream file;
    file.open(fileName, ios::out);
    if (!file.is_open())
    {
        cout << "Couldn't Open File.";
        exit(1);
    }
    for (int packet = 0; packet < numOfGeneratedPackets; packet++)
    {
        for (int byte = 0; byte < size; byte++)
        {
            file << hex
                 << uppercase
                 << setw(2)
                 << setfill('0')
                 << static_cast<int>(eth_GeneratedPackets[packet][byte]) << " ";
        }
        file << endl;
    }
    file.close();
}

void EthPacket::eth_convertToByteArray(uint64_t value, uint8_t size, uint8_t *byteArr)
{
    uint8_t mask = 0xff;
    for (int i = 0; i < size; i++)
    {
        byteArr[size - 1 - i] = ((value >> BITS_PER_BYTE * i)) & mask;
    }
}

uint32_t EthPacket::eth_calculate4ByteCRC(uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    uint32_t polynomial = 0xEDB88320;

    for (int i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ polynomial;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

void EthPacket::eth_writeIntoGeneratedPacketsDB(uint8_t *data, uint64_t size, uint64_t offset, string sel = "other")
{
    for (uint64_t packet = 0; packet < numOfNeededPackets; packet++)
    {
        if (sel == "crc")
        {
            eth_CRC = eth_calculate4ByteCRC(eth_GeneratedPackets[packet], PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + neededPayloadSize);
            eth_convertToByteArray(eth_CRC, CRC_SIZE, data);
        }
        else
        {
            // Do Nothing
        }
        for (uint64_t byte = 0; byte < size; byte++)
        {
            eth_GeneratedPackets[packet][offset + byte] = data[byte];
        }
    }
}

EthPacket::EthPacket(EthConfig *eth_p = NULL, ECPRI* ecpri_obj=NULL, string fileName = "Error")
{
    eth_packetIndex = 0;
    if (eth_p == NULL || ecpri_obj==NULL)
    {
        cout << "Please provide pointers to both Ethernet configuration structure and ECPRI class object which contains the ECPRI packets.";
        exit(1);
    }
    
    this->eth_p = eth_p;
    
    this->ecpri_obj = ecpri_obj;
    

    if (fileName == "Error")
    {
        cout << "Please provid a name for the output file.";
        exit(1);
    }
    outputFileName = fileName;
    outputFileHandler.open(fileName, ios::out);
    eth_ethernetParameterCalculations();
    eth_outputAllDetectedParametersToFile();
    eth_memoryAllocations();
    eth_fillGeneratedPackets();
    eth_fillEmptyPackets();
    eth_generateExtraBytesForInterruptedPacket();
    eth_sendDataPackets();
}

EthPacket::~EthPacket()
{
    delete[] eth_ExtraBytes;
    for (uint64_t i = 0; i < numOfGeneratedPackets; i++)
    {
        delete[] eth_GeneratedPackets[i];
    }
    delete[] eth_GeneratedPackets;
    delete[] eth_IFGsPerPacket;
    outputFileHandler.close();
}

void EthPacket::eth_ethernetParameterCalculations()
{
    generatedBytes = ceil(double(eth_p->LineRate) * double(eth_p->CaptureSizeMs) * 1000000.0 / 8.0);
    availablePayloadSize = eth_p->MaxPacketSize - (PREAMBLE_SFD_SIZE + ADDRESS_SIZE * 2 + ETHER_TYPE_SIZE + CRC_SIZE + eth_p->MinNumOfIFGsPerPacket);
    numOfIFGsToCompenstaeForSparedPayloadBytes = availablePayloadSize-ecpri_obj->ECPRIPacketSize;
    neededPayloadSize = ecpri_obj->ECPRIPacketSize;
    numOfExtraBytes = generatedBytes % eth_p->MaxPacketSize; // IFGs incase of interrupted packet generation
    numOfExtraBytes += numOfExtraBytes % 4;
    numOfGeneratedPackets = floor(double(generatedBytes) / double(eth_p->MaxPacketSize));
    numOfNeededPackets = ecpri_obj->numOfECPRIPackets;
    numOfEmptyPackets = numOfGeneratedPackets-numOfNeededPackets;
    numOfExtraIFGs = eth_p->MaxPacketSize % 4; // for allignment
    numOfTotalIFGsPerPacket = numOfExtraIFGs + eth_p->MinNumOfIFGsPerPacket + numOfIFGsToCompenstaeForSparedPayloadBytes;
    eth_PacketSize = PREAMBLE_SFD_SIZE + ADDRESS_SIZE * 2 + ETHER_TYPE_SIZE + CRC_SIZE+neededPayloadSize+numOfTotalIFGsPerPacket; 
    numOfIFGsInEmptyPackets = numOfEmptyPackets*eth_PacketSize; 
}

void EthPacket::eth_memoryAllocations()
{
    eth_ExtraBytes = new (nothrow) uint8_t[numOfExtraBytes];
    if (eth_ExtraBytes == nullptr)
    {
        cout << "Memory Failure 1";
        exit(1);
    }
    eth_GeneratedPackets = new (nothrow) uint8_t *[numOfGeneratedPackets];
    if (eth_GeneratedPackets == nullptr)
    {
        cout << "Memory Failure 3";
        exit(1);
    }
    for (uint64_t i = 0; i < numOfGeneratedPackets; i++)
    {
        eth_GeneratedPackets[i] = new (nothrow) uint8_t[eth_PacketSize];
        if (eth_GeneratedPackets[i] == nullptr)
        {
            cout << "Memory Failure 6";
            exit(1);
        }
    }
    eth_IFGsPerPacket = new (nothrow) uint8_t[numOfTotalIFGsPerPacket];
    if (eth_IFGsPerPacket == nullptr)
    {
        cout << "Memory Failure 8";
        exit(1);
    }
}

void EthPacket::eth_fillGeneratedPackets()
{
    eth_fillPreambleSFD();
    eth_fillDestAddress();
    eth_fillSourceAddress();
    eth_fillEtherType();
    eth_fillPayload();
    eth_fillCRC();
    eth_fillIFGs();
}

void EthPacket::eth_fillPreambleSFD()
{
    uint8_t *temp_ptr = new (nothrow) uint8_t[PREAMBLE_SFD_SIZE];
    if (temp_ptr == nullptr)
    {
        cout << "Memory Failure 4";
        exit(1);
    }
    eth_convertToByteArray(PREAMBLE_SFD, PREAMBLE_SFD_SIZE, temp_ptr);
    eth_writeIntoGeneratedPacketsDB(temp_ptr, PREAMBLE_SFD_SIZE, 0);

    delete[] temp_ptr;

    // Test
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble.txt", PREAMBLE_SFD_SIZE);
#endif
}

void EthPacket::eth_fillDestAddress()
{
    uint8_t *temp_ptr = new (nothrow) uint8_t[ADDRESS_SIZE];
    if (temp_ptr == nullptr)
    {
        cout << "Memory Failure 4";
        exit(1);
    }
    eth_convertToByteArray(eth_p->DestAddress, ADDRESS_SIZE, temp_ptr);
    eth_writeIntoGeneratedPacketsDB(temp_ptr, ADDRESS_SIZE, PREAMBLE_SFD_SIZE);

    delete[] temp_ptr;
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE);
#endif
}

void EthPacket::eth_fillSourceAddress()
{
    uint8_t *temp_ptr = new (nothrow) uint8_t[ADDRESS_SIZE];
    if (temp_ptr == nullptr)
    {
        cout << "Memory Failure 4";
        exit(1);
    }
    eth_convertToByteArray(eth_p->SourceAddress, ADDRESS_SIZE, temp_ptr);
    eth_writeIntoGeneratedPacketsDB(temp_ptr, ADDRESS_SIZE, PREAMBLE_SFD_SIZE + ADDRESS_SIZE);

    delete[] temp_ptr;
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE);
#endif
}

void EthPacket::eth_fillEtherType()
{
    uint8_t *temp_ptr = new (nothrow) uint8_t[ETHER_TYPE_SIZE];
    if (temp_ptr == nullptr)
    {
        cout << "Memory Failure 4";
        exit(1);
    }
    eth_convertToByteArray(eth_EtherType, ETHER_TYPE_SIZE, temp_ptr);
    eth_writeIntoGeneratedPacketsDB(temp_ptr, ETHER_TYPE_SIZE, PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE);

    delete[] temp_ptr;
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source_EtherType.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE);
#endif
}

void EthPacket::eth_fillPayload()
{
    int offset=PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE;
    for(int packet=0 ; packet<numOfNeededPackets ; packet++)
    {
        for(int byte=0 ; byte<neededPayloadSize; byte++)
        {
            eth_GeneratedPackets[packet][offset+byte]=ecpri_obj->ECPRI_GeneratedPackets[packet][byte];
        }
    }
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source_EtherType_Payload.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + neededPayloadSize);
#endif
}

void EthPacket::eth_fillCRC()
{
    uint8_t *temp_ptr = new (nothrow) uint8_t[CRC_SIZE];
    if (temp_ptr == nullptr)
    {
        cout << "Memory Failure 4";
        exit(1);
    }

    eth_writeIntoGeneratedPacketsDB(temp_ptr, CRC_SIZE, PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + neededPayloadSize, "crc");

    delete[] temp_ptr;
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source_EtherType_Payload_CRC.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + neededPayloadSize + CRC_SIZE);
#endif
}

void EthPacket::eth_fillIFGs()
{

    for (uint64_t i = 0; i < numOfTotalIFGsPerPacket; i++)
    {
        eth_IFGsPerPacket[i] = IFG;
    }

    eth_writeIntoGeneratedPacketsDB(eth_IFGsPerPacket, numOfTotalIFGsPerPacket, PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + neededPayloadSize + CRC_SIZE);
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source_EtherType_Payload_CRC_IFGs.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + neededPayloadSize + CRC_SIZE + numOfTotalIFGsPerPacket);
#endif
}

void EthPacket::eth_fillEmptyPackets()
{
    for(int packet=numOfNeededPackets; packet<numOfGeneratedPackets ; packet++)
    {
        for(int byte=0; byte<eth_PacketSize;byte++)
        {
            eth_GeneratedPackets[packet][byte]=IFG;
        }
    }
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Data_Packets_Plus_Empty_Packets.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + neededPayloadSize + CRC_SIZE + numOfTotalIFGsPerPacket);
#endif
}

void EthPacket::eth_generateExtraBytesForInterruptedPacket()
{
    for (uint64_t i = 0; i < numOfExtraBytes; i++)
    {
        eth_ExtraBytes[i] = IFG;
    }
}



void EthPacket::eth_sendPacket(uint8_t *dataPacket)
{
    for (uint64_t byte = 0; byte < eth_PacketSize; byte++)
    {
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(dataPacket[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(dataPacket[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(dataPacket[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(dataPacket[byte]) << endl;
    }
}


void EthPacket::eth_sendExtraBytes()
{
    for (uint64_t byte = 0; byte < numOfExtraBytes; byte++)
    {
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_ExtraBytes[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_ExtraBytes[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_ExtraBytes[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_ExtraBytes[byte]) << endl;
    }
}

void EthPacket::eth_sendDataPackets()
{
    for(int packet=0 ; packet<numOfGeneratedPackets;packet++)
    {
        eth_sendPacket(eth_GeneratedPackets[packet]);
    }
    eth_sendExtraBytes();
    cout<<"Packets has been generated and written into "<<outputFileName<<" successfully."<<endl;
}

void EthPacket::eth_outputAllDetectedParametersToFile()
{
    ofstream configFile("Detected Parameters/Detected_Configuration_Parameters.txt",ios::out);
    configFile<<"Ethernet Parameters:"<<endl
         << "\tLineRate = " <<eth_p->LineRate<<" Gbps"<<endl
         << "\tCaptureSizeMs = " <<eth_p->CaptureSizeMs<<" ms"<<endl
         << "\tMinNumOfIFGsPerPacket = " << eth_p->MinNumOfIFGsPerPacket<<endl
         << "\tDestAddress = "<<eth_p->DestAddress<<endl
         << "\tSourceAddress = "<<eth_p->SourceAddress<<endl
         << "\tMaxPacketSize = "<<eth_p->MaxPacketSize<<" Bytes"<<endl
         << "\tpacketSizeIncludingIFGsPerPacket = "<<eth_PacketSize<<" Bytes"<<endl
         << "\tnumOfGeneratedPackets = " <<numOfGeneratedPackets<< " Packets"<< endl
         << "\tnumOfNeededPackets = " << numOfNeededPackets<< " Packets"<<endl
         << "\tnumOfEmptyPackets = "<<numOfEmptyPackets<<" Packets"<<endl
         << "\tavailablePayloadSize = " << availablePayloadSize << " Bytes"<<endl
         << "\tneededPayloadSize = "<<neededPayloadSize<<" Bytes"<<endl
         << "\tnumOfIFGsToCompensateForTheEmptyPayloadBytes = "<<numOfIFGsToCompenstaeForSparedPayloadBytes<<endl
         << "\tnumOfExtraBytes(Due to packet generation interruption) = " <<numOfExtraBytes<<" Bytes"<< endl
         << "\tnumOfIFGsPerPacket = "<<numOfTotalIFGsPerPacket<<endl
         << "ECPRI Parameters:"<<endl
         << "\tnumOfECPRIPackets = "<<ecpri_obj->numOfECPRIPackets<<" Packets"<<endl
         << "\tECPRIPacketSize = "<<ecpri_obj->ECPRIPacketSize<<" Bytes"<<endl
         << "\tECPRIPayloadSize(ORAN Packet) = "<<(int)ecpri_obj->ecpriPayload<<" Bytes"<<endl
         << "ORAN Parameters:"<<endl
         << "\tSCS = "<<(int)ecpri_obj->oran_obj->oran_config->SCS<<" KHz"<<endl
         << "\tMaxNrbPerSymbol = "<<(int)ecpri_obj->oran_obj->oran_config->MaxNrb<<endl
         << "\tNrbPerPacket = "<<(int)ecpri_obj->oran_obj->oran_config->NrbPerPacket<<endl
         << "\tPayloadType = "<<ecpri_obj->oran_obj->oran_config->PayloadType<<endl
         << "\tIQSamplesFileName = "<<ecpri_obj->oran_obj->oran_config->payload<<endl
         << "\tnumOfIQSamplesInFile = "<<ecpri_obj->oran_obj->linesCounter<<endl
         << "\tnumOfFrames = "<<ecpri_obj->oran_obj->numOfFrames<<endl
         << "\tnumOfSubframes = "<<ecpri_obj->oran_obj->oran_config->numOfSubframes<<endl
         << "\tnumOfSlotsPerSubframe = "<<ecpri_obj->oran_obj->numOfSlotsPerSubframe<<endl
         << "\ttotalNumOfSlots = "<<ecpri_obj->oran_obj->numOfTotalSlots<<endl
         << "\tnumOfSymbolsPerSlot = "<<ecpri_obj->oran_obj->numOfSymbolsPerSlot<<endl
         << "\ttotalNumOfSymbols = "<<ecpri_obj->oran_obj->numOfTotalSymols<<endl
         << "\tnumOfNeededIQSamples = "<<ecpri_obj->oran_obj->numOfNeededIQSamples<<endl
         << "\tnumOfNeededPackets = "<<ecpri_obj->oran_obj->numOfNeededEthernetPackets<<endl
         << "\tapproximateNumOfPacketsPerFrame = "<<ecpri_obj->oran_obj->numOfPacketsPerFrame<<endl
         << "\tapproximateNumOfPacketsPerSubframe = "<<ecpri_obj->oran_obj->numOfPacketsPerSubframe<<endl
         << "\tapproximateNumOfPacketsPerSlot = "<<ecpri_obj->oran_obj->numOfPacketsPerSlot<<endl
         << "\tapproximateNumOfPacketsPerSymbol = "<<ecpri_obj->oran_obj->numOfPacketsPerSymbol<<endl;
    configFile.close();
}