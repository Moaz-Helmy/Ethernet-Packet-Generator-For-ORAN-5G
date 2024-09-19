#include "parser.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
using namespace std;

void Parser::parser_writeFile(string file, int sel)
{
    string *arr;
    if (sel == key)
    {
        arr = keys;
    }
    else
    {
        arr = values;
    }
    outFile.open(file, ios::out);
    for (int i = 0; i < linesCounter; i++)
    {
        outFile << arr[i] << endl;
    }
    outFile.close();
}

Parser::Parser(string fileName = "Error", EthConfig *ptr = NULL)
{
    p_config = ptr;
    this->fileName = fileName;
    if (this->fileName == "Error" || this->p_config == NULL)
    {
        cerr << "Parser Costructor:\n\tYou must provide output file name and pointer to configuration structure.";
        exit(1);
    }
    setupFile.open(this->fileName, ios::in);
    if (!setupFile.is_open())
    {
        cout << "Couldn't open file.";
        exit(1);
    }
    else
    {
        linesCounter = 0;
        while (getline(setupFile, temp))
        {
            linesCounter++;
        }
        setupFile.close();
        keys = new string[linesCounter];
        values = new string[linesCounter];
        convertedValues = new uint64_t[linesCounter];
        parser_extractSetupParameters();
    }
}
Parser::~Parser()
{
    delete[] keys;
    delete[] values;
    delete[] convertedValues;
}
string *Parser::parser_readFile()
{
    setupFile.open(fileName);
    int index = 0;
    while (getline(setupFile, temp))
    {
        keys[index] = temp;
        index++;
    }
#ifdef DEBUG_MODE_ON
    parser_writeFile("Parser Debug Files/parser_readData.txt", key);
#endif
    return keys;
}
string *Parser::parser_removeComments()
{
    int size = 0;
    for (int i = 0; i < linesCounter; i++)
    {
        size = keys[i].size();
        for (int j = 0; j <= size; j++)
        {
            if (keys[i][j] == '/')
            {
                keys[i] = keys[i].substr(0, j - 1);
                break;
            }
            else if (keys[i][j] == '\0')
            {
                break;
            }
            else
            {
                // Do Nothing
            }
        }
    }
#ifdef DEBUG_MODE_ON
    parser_writeFile("Parser Debug Files/parser_noComments.txt", key);
#endif
    return keys;
}

void Parser::parser_extractKeysAndValues(string *keys = NULL, string *values = NULL)
{
    int line = 0;
    int i = 0;
    int size = 0;
    for (line = 0; line < linesCounter; line++)
    {
        size = this->keys[line].size();
        for (i = 0; i < size; i++)
        {
            if (this->keys[line][i] == '=')
            {
                this->values[line] = this->keys[line].substr(i + 1, size - 1 - i);
                this->keys[line] = this->keys[line].substr(0, i - 1);
                break;
            }
        }
    }
    if (keys != NULL && values != NULL)
    {
        keys = this->keys;
        values = this->values;
    }
#ifdef DEBUG_MODE_ON
    parser_writeFile("Parser Debug Files/parser_keys.txt", key);
    parser_writeFile("Parser Debug Files/parser_values.txt", value);
#endif
    return;
}
void Parser::parser_removeSpaces(string *keys = NULL, string *values = NULL)
{
    int initialSpacesCount = 0;
    int size_key = 0;
    int size_val = 0;
    bool flag_key = 0;
    bool flag_val = 0;
    int line = 0;
    int i_key = 0;
    int i_val = 0;
    for (line = 0; line < linesCounter; line++)
    {
        initialSpacesCount = 0;
        size_key = this->keys[line].size();
        temp = this->keys[line];
        flag_key = 0;
        for (i_key = 0; i_key < size_key; i_key++)
        {
            if (temp[i_key] != ' ' && !flag_key)
            {
                flag_key = 1;
            }
            if (temp[i_key] == ' ' && !flag_key)
            {
                initialSpacesCount++;
                this->keys[line] = temp.substr(i_key + 1, size_key - 1 - i_key + 1 - 1);
            }
            else if (temp[i_key] == ' ' && flag_key)
            {
                this->keys[line] = this->keys[line].substr(0, i_key - initialSpacesCount);
                break;
            }
            else
            {
                // Do Nothing
            }
        }

        initialSpacesCount = 0;
        size_val = this->values[line].size();
        temp = this->values[line];
        flag_val = 0;
        for (i_val = 0; i_val < size_val; i_val++)
        {
            if (temp[i_val] != ' ' && !flag_val)
            {
                flag_val = 1;
            }
            if (temp[i_val] == ' ' && !flag_val)
            {
                initialSpacesCount++;
                this->values[line] = temp.substr(i_val + 1, size_val - 1 - i_val + 1 - 1);
            }
            else if (temp[i_val] == ' ' && flag_val)
            {
                this->values[line] = this->values[line].substr(0, i_val - initialSpacesCount);
                break;
            }
            else
            {
                // Do Nothing
            }
        }
    }
    if (keys != NULL && values != NULL)
    {
        keys = this->keys;
        values = this->values;
    }
#ifdef DEBUG_MODE_ON
    parser_writeFile("Parser Debug Files/parser_keysNoSpaces.txt", key);
    parser_writeFile("Parser Debug Files/parser_valuesNoSpaces.txt", value);
#endif
}

uint64_t *Parser::parser_convertToValue()
{
    uint64_t t;
    for (int line = 0; line < linesCounter; line++)
    {
        if (values[line][1] == 'x')
        {
            t = stoll(values[line].substr(2, values[line].size() - 2), nullptr, 16);
            convertedValues[line] = t;
        }
        else
        {
            t = stoll(values[line]);
            convertedValues[line] = t;
        }
    }
    return convertedValues;
}

void Parser::parser_setValuesToConfigurationStructure()
{

    for (int line = 0; line < linesCounter; line++)
    {
        if (keys[line] == "Eth.LineRate")
        {
            p_config->LineRate = (uint16_t)convertedValues[line];
        }
        else if (keys[line] == "Eth.CaptureSizeMs")
        {
            p_config->CaptureSizeMs = (uint16_t)convertedValues[line];
        }
        else if (keys[line] == "Eth.MinNumOfIFGsPerPacket")
        {
            p_config->MinNumOfIFGsPerPacket = (uint16_t)convertedValues[line];
        }
        else if (keys[line] == "Eth.DestAddress")
        {
            p_config->DestAddress = convertedValues[line];
        }
        else if (keys[line] == "Eth.SourceAddress")
        {
            p_config->SourceAddress = convertedValues[line];
        }
        else if (keys[line] == "Eth.MaxPacketSize")
        {
            p_config->MaxPacketSize = (uint16_t)convertedValues[line];
        }
        else if (keys[line] == "Eth.BurstSize")
        {
            p_config->BurstSize = (uint16_t)convertedValues[line];
        }
        else if (keys[line] == "Eth.BurstPeriodicity_us")
        {
            p_config->BurstPeriodicity_us = (uint16_t)convertedValues[line];
        }
    }
}
void Parser::parser_extractSetupParameters()
{
    parser_readFile();
    parser_removeComments();
    parser_extractKeysAndValues();
    parser_removeSpaces();
    parser_convertToValue();
    parser_setValuesToConfigurationStructure();
}
