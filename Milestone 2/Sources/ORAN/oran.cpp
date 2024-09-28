#include "oran.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
using namespace std;

ORAN::ORAN(ORANConfig *oran_ptr = NULL)
{
    if (oran_ptr == NULL)
    {
        cout << "Please provide pointer to ORAN configuration structure.";
        exit(1);
    }
    else
    {
        oran_config = oran_ptr;
    }

    ORAN_checkPayloadTypeAndAllocateMemory();
    ORAN_readIQSamples();
    ORAN_parametersCalculations();
    ORAN_memoryAllocation();
    ORAN_fillPacketsHeader();
    ORAN_fillPayload();
}

ORAN::~ORAN()
{
    delete[] I;
    delete[] Q;
    delete[] fileData;
    for (int packet = 0; packet < numOfNeededEthernetPackets; packet++)
    {
        delete[] ORAN_GeneratedPackets[packet];
    }
    delete[] ORAN_GeneratedPackets;
}

void ORAN::ORAN_checkPayloadTypeAndAllocateMemory()
{
    if (oran_config->PayloadType == "fixed" || oran_config->PayloadType == "Fixed")
    {

        IQFile.open(oran_config->payload, ios::in);
        if (!IQFile.is_open())
        {
            cout << "Couldn't open file.";
            exit(1);
        }
        linesCounter = 0;
        while (getline(IQFile, temp))
        {
            linesCounter++;
        }
        IQFile.close();
        I = new short[linesCounter];
        Q = new short[linesCounter];
        fileData = new string[linesCounter];
    }
    else
    {
        cout << "No IQ samples.";
        exit(1);
        // Do Nothing for now
    }
}

void ORAN::ORAN_readIQSamples()
{
    int line;
    string tempI, tempQ;
    IQFile.open(oran_config->payload, ios::in);
    if (!IQFile.is_open())
    {
        cout << "Couldn't open file.";
        exit(1);
    }
    for (line = 0; line < linesCounter; line++)
    {
        getline(IQFile, fileData[line]);
        // cout<<fileData[line]<<endl;

        /*
            Separaete I and Q
        */
        int spaceIndex = 0;
        for (int i = 0; i < fileData[line].size(); i++)
        {
            if (fileData[line][i] == ' ')
            {
                spaceIndex = i;
            }
        }
        tempI = fileData[line].substr(0, spaceIndex);
        tempQ = fileData[line].substr(spaceIndex + 1, fileData[line].size() - spaceIndex + 1);
        I[line] = (short)stoi(tempI);
        Q[line] = (short)stoi(tempQ);
         //cout<<"I = "<<I[line]<<endl<<"Q = "<<Q[line]<<endl;
    }
    IQFile.close();
}

void ORAN::ORAN_parametersCalculations()
{

    cout << (int)oran_config->MaxNrb << endl
         << (int)oran_config->NrbPerPacket << endl
         << (int)oran_config->numOfSubframes << endl
         << oran_config->payload << endl
         //<<(int)oran_config->payloadParameterLine<<endl
         << oran_config->PayloadType << endl
         << (int)oran_config->SCS << endl;
    numOfSlotsPerSubframe = oran_config->SCS / MINIMAL_SCS;
    numOfTotalSlots = numOfSlotsPerSubframe * oran_config->numOfSubframes;
    numOfSymbolsPerSlot = SYMBOLS_PER_SLOT_NORMAL_CP;
    numOfTotalSymols = numOfSymbolsPerSlot * numOfTotalSlots;
    numOfNeededIQSamples = oran_config->MaxNrb * IQ_SAMPLES_PER_RB;
    numOfNeededEthernetPackets = ceil((double)(oran_config->MaxNrb * numOfTotalSymols) / (double)(oran_config->NrbPerPacket));
    ORANPacketHeaderSize = ORAN_HEADER_SIZE;
    numOfIQSamplesPerPacket = oran_config->NrbPerPacket * IQ_SAMPLES_PER_RB;
    ORANPacketSize = ORAN_HEADER_SIZE + RB_HEADER_SIZE + numOfIQSamplesPerPacket*2*SAMPLE_BYTES;
    numOfFrames = ceil((double)oran_config->numOfSubframes / (double)10);
    numOfSlotsPerFrame = numOfSlotsPerSubframe * 10;
    numOfSymbolsPerFrame = numOfSymbolsPerSlot * numOfSlotsPerFrame;
    numOfNrbsPerFrame = oran_config->MaxNrb * numOfSymbolsPerFrame;
    numOfPacketsPerFrame = ceil((double)numOfNrbsPerFrame / (double)oran_config->NrbPerPacket);
    numOfPacketsPerSubframe = ceil((double)(numOfSlotsPerSubframe * numOfSymbolsPerSlot * oran_config->MaxNrb) / (double)(oran_config->NrbPerPacket));
    numOfErrorPacketsPerSubframe = numOfPacketsPerSubframe * SUBFRAMES_PER_FRAME - numOfPacketsPerFrame;
    numOfPacketsPerSlot = ceil((double)(numOfSymbolsPerSlot * oran_config->MaxNrb) / (double)(oran_config->NrbPerPacket));
    numOfErrorPacketsPerSlot = numOfPacketsPerSlot * numOfSlotsPerSubframe - numOfPacketsPerSubframe;
    numOfPacketsPerSymbol = ceil((double)oran_config->MaxNrb / (double)oran_config->NrbPerPacket);
    numOfErrorPacketsPerSymbol = numOfPacketsPerSymbol * numOfSymbolsPerSlot - numOfPacketsPerSlot;
    cout << numOfPacketsPerFrame << endl
         << numOfFrames << endl
         << numOfNeededEthernetPackets << endl
         << numOfPacketsPerSubframe << endl
         << numOfPacketsPerSlot << endl
         << numOfPacketsPerSymbol << endl
         << numOfSlotsPerSubframe << endl
         << oran_config->numOfSubframes << endl
         << numOfErrorPacketsPerSlot << endl
         << numOfErrorPacketsPerSymbol << endl
         << linesCounter <<endl
         << (int)I[677310-1]<<endl;
}

void ORAN::ORAN_memoryAllocation()
{
    ORAN_GeneratedPackets = new (nothrow) uint8_t *[numOfNeededEthernetPackets];
    if (ORAN_GeneratedPackets == nullptr)
    {
        cout << "Memory Failure ORAN.";
        exit(1);
    }
    for (int packet = 0; packet < numOfNeededEthernetPackets; packet++)
    {
        ORAN_GeneratedPackets[packet] = new (nothrow) uint8_t[ORANPacketSize];
        if (ORAN_GeneratedPackets[packet] == nullptr)
        {
            cout << "Memory Failure ORAN.";
            exit(1);
        }
    }

    ORAN_AllPacketsConcatenated = new (nothrow) uint8_t[ORANPacketSize * numOfNeededEthernetPackets];
    if (ORAN_AllPacketsConcatenated == nullptr)
    {
        cout << "Memory Failure ORAN.";
        exit(1);
    }
}

void ORAN::ORAN_fillPacketsHeader()
{
    // First Byte
    for (int packet = 0; packet < numOfNeededEthernetPackets; packet++)
    {

        ORANHeader_struct.firstByte = 0x00;
        ORAN_GeneratedPackets[packet][0] = ORANHeader_struct.firstByte;
    }

    // FrameID and SubFrameID
    int packet = 0;
    for (int frame = 0; frame < numOfFrames; frame++)
    {
        for (packet = 0; packet < numOfPacketsPerFrame; packet++)
        {
            int t = packet + frame * numOfPacketsPerFrame;
            if (t >= numOfNeededEthernetPackets)
            {
                break;
            }
            else
            {
                ORAN_GeneratedPackets[t][1] = (uint8_t)frame;
            }
        }
        for (int subframe = 0; subframe < SUBFRAMES_PER_FRAME; subframe++)
        {
            int end = 0;
            if (subframe == SUBFRAMES_PER_FRAME - 1)
            {
                end = numOfPacketsPerSubframe - numOfErrorPacketsPerSubframe;
            }
            else
            {
                end = numOfPacketsPerSubframe;
            }
            for (packet = 0; packet < end; packet++)
            {
                int t = packet + subframe * numOfPacketsPerSubframe + frame * numOfPacketsPerFrame;
                if (t >= numOfNeededEthernetPackets)
                {
                    break;
                }
                ORAN_GeneratedPackets[t][2] = (uint8_t)subframe;
            }
        }
    }

    // Slot ID
    for (int subframe = 0; subframe < oran_config->numOfSubframes; subframe++)
    {
        for (int slot = 0; slot < numOfSlotsPerSubframe; slot++)
        {
            int end = 0;
            if (slot == numOfSlotsPerSubframe - 1)
            {
                end = numOfPacketsPerSlot - numOfErrorPacketsPerSlot;
            }
            else
            {
                end = numOfPacketsPerSlot;
            }
            for (int packet = 0; packet < end; packet++)
            {
                int t = packet + slot * numOfPacketsPerSlot + subframe * numOfPacketsPerSubframe;
                if (t >= numOfNeededEthernetPackets)
                {
                    break;
                }
                ORAN_GeneratedPackets[t][3] = (uint8_t)slot;
            }
        }
    }
    
    //Symbol Id
    for (int slot = 0; slot < numOfTotalSlots; slot++)
    {
        for (int symbol = 0; symbol < numOfSymbolsPerSlot; symbol++)
        {
            for (int packet = 0; packet < numOfPacketsPerSymbol; packet++)
            {
                int t = packet + symbol * numOfPacketsPerSymbol + slot * numOfPacketsPerSlot;
                if (t >= numOfNeededEthernetPackets)
                {
                    break;
                }
                ORAN_GeneratedPackets[t][4] = (uint8_t)symbol;
            }
        }
    }
    for (int subframe = 0; subframe < oran_config->numOfSubframes; subframe++)
    {
        for (int packet = 0; packet < subframe; packet++)
        {
            int t = packet + subframe * numOfPacketsPerSubframe;
            if (t >= numOfNeededEthernetPackets)
            {
                break;
            }
            ORAN_GeneratedPackets[t][4] = 0;
        }
    }

    ORAN_adjustHeaderBytes();
    ORAN_writeGeneratedPacketsToFile();
}

void ORAN::ORAN_adjustHeaderBytes()
{
    for (int packet = 0; packet < numOfNeededEthernetPackets; packet++)
    {
        int slotID = ORAN_GeneratedPackets[packet][3];
        int subframeID = ORAN_GeneratedPackets[packet][2];
        ORAN_GeneratedPackets[packet][2] = (subframeID << 4) + slotID;
        ORAN_GeneratedPackets[packet][3] = ORAN_GeneratedPackets[packet][4];
        ORAN_GeneratedPackets[packet][4] = 0;
    }
}

void ORAN::ORAN_fillPayload()
{
    uint8_t startPrbu = 0;
    uint8_t numPrbu = oran_config->NrbPerPacket;
    uint64_t sample_index=0;
    uint32_t byte_index=8;
    uint8_t* byteArrI=new(nothrow) uint8_t[SAMPLE_BYTES];
    if(byteArrI==nullptr)
    {
        cout<<"Memory Failure.";
        exit(1);
    }
    uint8_t* byteArrQ=new(nothrow) uint8_t[SAMPLE_BYTES];
    if(byteArrQ==nullptr)
    {
        cout<<"Memory Failure.";
        exit(1);
    }
    for(int packet=0; packet<numOfNeededEthernetPackets;packet++)
    {
        ORAN_GeneratedPackets[packet][5]=0;
        ORAN_GeneratedPackets[packet][6]= startPrbu+packet*oran_config->NrbPerPacket;
        ORAN_GeneratedPackets[packet][7]= numPrbu;
        byte_index=8;
        for(int sample=0 ; sample<numOfIQSamplesPerPacket;sample++)
        {
            ORAN_convertToByteArray(I[sample_index],SAMPLE_BYTES,byteArrI);
            ORAN_convertToByteArray(Q[sample_index],SAMPLE_BYTES,byteArrQ);
            
            for(int byte=0 ; byte<SAMPLE_BYTES ; byte++)
            {
                //cout<<hex<<uppercase<<setw(2)<<setfill('0')<<static_cast<int>(byteArrI[byte])<<" "<<static_cast<int>(byteArrI[byte])<<endl;
                ORAN_GeneratedPackets[packet][byte_index]=byteArrI[byte];
                byte_index++;
            }
            for(int byte=0 ; byte<SAMPLE_BYTES ; byte++)
            {
                ORAN_GeneratedPackets[packet][byte_index]=byteArrQ[byte];
                byte_index++;
            }
            sample_index++;
            if(sample_index>linesCounter)
            {
                sample_index=0;
            }
        }
    }
    ORAN_writeGeneratedPacketsToFile();
    delete[] byteArrI;
    delete[] byteArrQ;
}

void ORAN::ORAN_writeGeneratedPacketsToFile()
{
    ofstream outFile("Generated_ORAN_Packets.txt", ios::out);
    if (!outFile.is_open())
    {
        cout << "Couldn't Open File.";
        exit(1);
    }
    for (int packet = 0; packet < numOfNeededEthernetPackets; packet++)
    {
        for (int byte = 0; byte < ORANPacketSize; byte++)
        {
            outFile << hex
                    << uppercase
                    << setw(2)
                    << setfill('0')
                    << static_cast<int>(ORAN_GeneratedPackets[packet][byte]) << " ";
        }
        outFile << endl;
    }

    outFile.close();
}

void ORAN::ORAN_convertToByteArray(uint64_t value, uint8_t size, uint8_t *byteArr)
{
    uint8_t mask = 0xff;
    for (int i = 0; i < size; i++)
    {
        byteArr[size - 1 - i] = ((value >> BITS_PER_BYTE * i)) & mask;
    }
}
