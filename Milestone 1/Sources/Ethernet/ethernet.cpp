/*
    Author: Moaz Mohamed 
    Description: This is the source file for the ethernet class
    Date: 19/9/2024
*/
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

void EthPacket::eth_randomizePayload()
{
    srand(static_cast<unsigned>(15));
    for (int i = 0; i < payloadSize; i++)
    {
        eth_Payload[i] = uint8_t(rand());
    }
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

void EthPacket::eth_writeIntoGeneratedPacketsDB(uint8_t *data, uint16_t size, uint16_t offset, string sel = "other")
{
    for (uint64_t packet = 0; packet < numOfGeneratedPackets; packet++)
    {
        if (sel == "payload")
        {
            eth_randomizePayload();
        }
        else if (sel == "crc")
        {
            eth_CRC = eth_calculate4ByteCRC(eth_GeneratedPackets[packet], PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + payloadSize);
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

EthPacket::EthPacket(EthConfig *eth_p = NULL, string fileName = "Error")
{
    eth_packetIndex = 0;
    if (eth_p == NULL)
    {
        cout << "Please provide pointer to configuration structure.";
        exit(1);
    }
    this->eth_p = eth_p;
    eth_PacketSize = eth_p->MaxPacketSize;

    if (fileName == "Error")
    {
        cout << "Please provid a name for the output file.";
        exit(1);
    }
    outputFileHandler.open(fileName, ios::out);
    eth_ethernetParameterCalculations();
    eth_memoryAllocations();
    eth_fillGeneratedPackets();
    eth_generateExtraBytesForInterruptedPacket();
    eth_generateIdleBytes();
    eth_sendAll();
}

EthPacket::~EthPacket()
{
    delete[] eth_ExtraBytes;
    delete[] eth_IdleBytes;
    for (uint64_t i = 0; i < numOfGeneratedPackets; i++)
    {
        delete[] eth_GeneratedPackets[i];
    }
    delete[] eth_GeneratedPackets;
    delete[] eth_Payload;
    delete[] eth_IFGsPerPacket;
    outputFileHandler.close();
}

void EthPacket::eth_ethernetParameterCalculations()
{
    generatedBytes = ceil(double(eth_p->LineRate) * double(eth_p->CaptureSizeMs) * 1000000.0 / 8.0);
    numOfExtraBytes = generatedBytes % eth_p->MaxPacketSize; // IFGs incase of interrupted packet generation
    numOfExtraBytes += numOfExtraBytes % 4;
    numOfGeneratedPackets = floor(double(generatedBytes) / double(eth_p->MaxPacketSize));
    numOfExtraIFGs = eth_p->MaxPacketSize % 4; // for allignment
    numOfTotalIFGs = numOfExtraIFGs + eth_p->MinNumOfIFGsPerPacket;
    payloadSize = eth_p->MaxPacketSize - (PREAMBLE_SFD_SIZE + ADDRESS_SIZE * 2 + ETHER_TYPE_SIZE + CRC_SIZE + eth_p->MinNumOfIFGsPerPacket);
    numOfPacketsPerPeriod = floor(double(eth_p->LineRate) * double(eth_p->BurstPeriodicity_us) * 125.0 / double(eth_p->MaxPacketSize));
    numOfIdlePackets = numOfPacketsPerPeriod - eth_p->BurstSize;
    numOfIdleBytes = (eth_p->LineRate * eth_p->BurstPeriodicity_us * 125) % (eth_p->MaxPacketSize);
    numOfTotalIdleBytesPerPeriod = numOfIdlePackets * eth_p->MaxPacketSize + numOfIdleBytes;
    numOfTotalIdleBytesPerPeriod -= numOfTotalIdleBytesPerPeriod % 4; // 4 byte allignment
    numOfPacketsInLastBurst = numOfGeneratedPackets % eth_p->BurstSize;
    numOfCompleteBursts = numOfGeneratedPackets / eth_p->BurstSize;

#ifdef DEBUG_MODE_ON
    ofstream configFile("Detected_Configuration_Parameters.txt",ios::out);
    configFile << "LineRate = " <<eth_p->LineRate<<" Gbps"<<endl
         << "CaptureSizeMs = " <<eth_p->CaptureSizeMs<<" ms"<<endl
         << "MinNumOfIFGsPerPacket = " << eth_p->MinNumOfIFGsPerPacket<<endl
         << "DestAddress = "<<eth_p->DestAddress<<endl
         << "SourceAddress = "<<eth_p->SourceAddress<<endl
         << "MaxPacketSize = "<<eth_p->MaxPacketSize<<" Bytes"<<endl
         << "BurstSize = "<<eth_p->BurstSize<<" Packets"<<endl
         << "BursPeriodicity_us = "<<eth_p->BurstPeriodicity_us<<" us"<<endl
         << "numOfGeneratedPackets = " <<numOfGeneratedPackets<< " Packets"<< endl
         << "payloadSize = " << payloadSize << " Bytes"<<endl
         << "numOfPacketsPerPeriod = " << numOfPacketsPerPeriod<<" Packets"<< endl
         << "numOfIdlePackets = " << numOfIdlePackets <<" Packets"<<endl
         << "numOfTotalIdleBytesPerPeriod = " <<numOfTotalIdleBytesPerPeriod<<" Bytes" <<endl
         << "numOfExtraBytes = " <<numOfExtraBytes<<" Bytes"<< endl
         << "numOfIdleBytes = "<<numOfIdleBytes<<" Bytes"<< endl
         << "numOfPacketsInLastBurst = " <<numOfPacketsInLastBurst<<" Packets" <<endl
         << "numOfCompleteBursts = " <<numOfCompleteBursts<<" Bursts" <<endl;
    configFile.close();
#endif
    assert((numOfPacketsInLastBurst + numOfCompleteBursts * 3) == numOfGeneratedPackets);
}

void EthPacket::eth_memoryAllocations()
{
    eth_ExtraBytes = new (nothrow) uint8_t[numOfExtraBytes];
    if (eth_ExtraBytes == nullptr)
    {
        cout << "Memory Failure 1";
        exit(1);
    }
    eth_IdleBytes = new (nothrow) uint8_t[numOfTotalIdleBytesPerPeriod];
    if (eth_IdleBytes == nullptr)
    {
        cout << "Memory Failure 2";
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
    eth_Payload = new (nothrow) uint8_t[payloadSize];
    if (eth_Payload == nullptr)
    {
        cout << "Memory Failure 7";
        exit(1);
    }
    eth_IFGsPerPacket = new (nothrow) uint8_t[numOfTotalIFGs];
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

    eth_writeIntoGeneratedPacketsDB(eth_Payload, payloadSize, PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE, "payload");
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source_EtherType_Payload.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + payloadSize);
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

    eth_writeIntoGeneratedPacketsDB(temp_ptr, CRC_SIZE, PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + payloadSize, "crc");

    delete[] temp_ptr;
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source_EtherType_Payload_CRC.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + payloadSize + CRC_SIZE);
#endif
}

void EthPacket::eth_fillIFGs()
{

    for (uint16_t i = 0; i < numOfTotalIFGs; i++)
    {
        eth_IFGsPerPacket[i] = IFG;
    }

    eth_writeIntoGeneratedPacketsDB(eth_IFGsPerPacket, numOfTotalIFGs, PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + payloadSize + CRC_SIZE);
#ifdef DEBUG_MODE_ON
    eth_writeGeneratedPackets("Ethernet Debug Files/Generated_Preamble_Destination_Source_EtherType_Payload_CRC_IFGs.txt", PREAMBLE_SFD_SIZE + ADDRESS_SIZE + ADDRESS_SIZE + ETHER_TYPE_SIZE + payloadSize + CRC_SIZE + numOfTotalIFGs);
#endif
}

void EthPacket::eth_generateExtraBytesForInterruptedPacket()
{
    for (uint16_t i = 0; i < numOfExtraBytes; i++)
    {
        eth_ExtraBytes[i] = IFG;
    }
}

void EthPacket::eth_generateIdleBytes()
{
    for (uint16_t i = 0; i < numOfTotalIdleBytesPerPeriod; i++)
    {
        eth_IdleBytes[i] = IFG;
    }
}

void EthPacket::eth_sendPacket(uint8_t *dataPacket)
{
    for (uint16_t byte = 0; byte < eth_PacketSize; byte++)
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
void EthPacket::eth_sendIdleBytes()
{
    for (uint16_t byte = 0; byte < numOfTotalIdleBytesPerPeriod; byte++)
    {
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_IdleBytes[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_IdleBytes[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_IdleBytes[byte]) << " ";
        byte++;
        outputFileHandler << hex
                          << uppercase
                          << setw(2)
                          << setfill('0')
                          << static_cast<int>(eth_IdleBytes[byte]) << endl;
    }
}

void EthPacket::eth_sendExtraBytes()
{
    for (uint16_t byte = 0; byte < numOfExtraBytes; byte++)
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

void EthPacket::eth_sendAll()
{
    eth_sendCompleteBursts();
    eth_sendLastBurtAndExtraBytes();
}

void EthPacket::eth_sendCompleteBursts()
{
    for (uint32_t burst = 0; burst < numOfCompleteBursts; burst++)
    {
        eth_sendPacket(eth_GeneratedPackets[eth_packetIndex]);
        eth_packetIndex++;
        eth_sendPacket(eth_GeneratedPackets[eth_packetIndex]);
        eth_packetIndex++;
        eth_sendPacket(eth_GeneratedPackets[eth_packetIndex]);
        eth_packetIndex++;

        eth_sendIdleBytes();
    }
}

void EthPacket::eth_sendLastBurtAndExtraBytes()
{
    for (uint16_t packet = 0; packet < numOfPacketsInLastBurst; packet++)
    {
        eth_sendPacket(eth_GeneratedPackets[eth_packetIndex]);
        eth_packetIndex++;
    }
    if (numOfPacketsInLastBurst == eth_p->BurstSize)
    {
        eth_sendIdleBytes();
        eth_sendExtraBytes();
    }
    else
    {
        eth_sendExtraBytes();
    }
}
