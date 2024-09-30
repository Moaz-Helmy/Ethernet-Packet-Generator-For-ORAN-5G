/*
    Author: Moaz Mohamed
    Description: This is the main source file to run the program (Ethernet Packet Generator For 
                Fronthaul interface between DU and RU in ORAN 5G NR)
    Date: 27/9/2024
*/

/************************************************************************
 ****************************List Of Includes****************************
 ************************************************************************/

/*
    Include the input out streames
*/
#include <iostream>

/*
    Include the header file of the parser class to be able to read the configuration
    parameters of ethernet and ORAN and ECPRI
*/
#include "Sources/Parser/parser.h"

/*
    Include the ethernet class that's responsible for the generation of the final
    packets that are ready for transmission
*/
#include "Sources/Ethernet/ethernet.h"

/*
    Include the oran class for oran packet generation
*/
#include "Sources/ORAN/oran.h"

/*
    Include the ecpri class for ecpri packet generation
*/
#include "Sources/ECPRI/ecpri.h"

/*
    Use std as a namespace to faciliatate usage of streams
*/
using namespace std;

/************************************************************************
 ****************************Start the Program***************************
 ************************************************************************/
/*
    Arguments Description:
        - argc is the number of arguements that are passed to the program when 
          executed.
        - argv is an array of strings which stores the arguments passed to the 
          program.
          Note that: -->argv[1] is the configuration file (second_milestone.txt)
                     -->argv[2] is the output file that include the generated ethernet
                        packets arranged in a 4-byte oriented manner per line.

*/
int main(int argc, char *argv[])
{
    /*
        Create ethernet configuration structure that stores the ethernet configuration 
        parameters.
    */
    EthConfig Eth;

    /*
        Create oran configuration structure that stores the oran configuration parameters
    */
    ORANConfig Oran;
    
    /*
        Create the parser object and pass the configuration file, the ethernet configuration
        structure and the oran configuration structure to extract the parameters from the file 
        and store them in the configuration structures.
    */
    Parser parse_obj(argv[1], &Eth, &Oran);

    /*
        Create the ORAN object and pass the oran configuration structure of the oran to it
        to generate the oran packets.
    */
    ORAN oran_obj(&Oran);

    /*
        Create ECPRI object and pass the generated ORAN packets to it throuh passing a pointer 
        to the oran object that includes the generated ORAN packets.
        Then the ECPRI object generates the ECPRI Packets via attaching the ECPRI Header into 
        the ORAN Packets which are the Payload of the ECPRI Packets.
    */
    ECPRI ecpri_obj(&oran_obj);

    /*
        Create the ethernet object which is responsible for:
        - Generate the ethernet packets via attaching the ethernet header and tail to recieved 
          ecpri packets. That's why the ecpri object is passed to it.
        - Adjust the generation of packets according to the configuration parameters passed in 
          the ethernet configuration structure.
        - Write the generated ethernet packets into the output file argv[2] in a 4-byte oriented
          manner.
    */
    EthPacket obj(&Eth,&ecpri_obj, argv[2]);
    
    return 0;
}