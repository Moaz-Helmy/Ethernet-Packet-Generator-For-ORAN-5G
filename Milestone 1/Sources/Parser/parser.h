/*
    Author: Moaz Mohamed 
    Description: This is the header file for the parser class
    Date: 19/9/2024
*/

/**************File Guard*******************/
#ifndef PARSER_H
#define PARSER_H

/**************Necessary Includes*******************/

/*
    Include ethernet.h to be able to define the EthConfig Structure
*/
#include "../Ethernet/ethernet.h"

/*
    Included for file processing
*/
#include <fstream>
#include <iostream>
using namespace std;


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
    EthConfig *p_config;
    void parser_writeFile(string, int);

public:
    Parser(string, EthConfig *);
    ~Parser();
    string *parser_readFile();
    string *parser_removeComments();
    void parser_extractKeysAndValues(string *, string *);
    void parser_removeSpaces(string *, string *);
    uint64_t *parser_convertToValue();
    void parser_setValuesToConfigurationStructure();
    void parser_extractSetupParameters();
};

#endif