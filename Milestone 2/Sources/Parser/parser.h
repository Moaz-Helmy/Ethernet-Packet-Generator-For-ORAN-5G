#ifndef PARSER_H
#define PARSER_H

#include "../Ethernet/ethernet.h"
#include "../ORAN/oran.h"
#include <fstream>
#include <iostream>
using namespace std;

#define DEBUG_MODE_ON
class Parser
{
    enum selection
    {
        key,
        value
    } sel;
    ifstream setupFile;
    string *keys;
    string *values;
    uint64_t *convertedValues;
    string temp;
    string fileName;
    int linesCounter;
    ofstream outFile;
    EthConfig *eth_config;
    ORANConfig *oran_config;
    void parser_writeFile(string, int);

public:
    Parser(string, EthConfig *,ORANConfig*);
    ~Parser();
    string *parser_readFile();
    string *parser_removeComments();
    void parser_extractKeysAndValues(string *, string *);
    void parser_removeSpaces(string *, string *);
    void parser_getORANPayloadParameterLine();
    uint64_t *parser_convertToValue();
    void parser_setValuesToConfigurationStructure();
    void parser_extractSetupParameters();
    void parser_readIQSamples();
};

#endif