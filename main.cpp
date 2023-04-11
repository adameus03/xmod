#include <iostream>
#include <cstdlib>
#include <windows.h>
#include "transfer.h"

#define DIR_INSIDE 0x0
#define DIR_OUTSIDE 0x1

void usage(char* argv0){
    std::cout << "Usage: " << argv0 << " [--rx] [--tx] <port> <baudRate> <file> [--crc16]" << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "xmod (XMODEM file transfer tool via serial port)" << std::endl;

    unsigned char transmission_direction;

    if(argc == 5 || argc==6){

        if(!strcmp("--rx", argv[1])){
            transmission_direction = DIR_INSIDE;
        }
        else if(!strcmp("--tx", argv[1])){
            transmission_direction = DIR_OUTSIDE;
        }
        else { usage(*argv); return 0; }
    }
    else { usage(*argv); return 0; }

    if(argc == 6){
        if(strcmp("--crc16", argv[5])) { usage(*argv); return 0; }
    }

    try {
        HANDLE comm = get_comm(argv[2], atoi(argv[3]));
        if(transmission_direction == DIR_INSIDE){
            receive_file(comm, argv[4]);
        }
        else if(argc==6){
            transmit_file(comm, argv[4], TRANSMIT_MODE::CRC_16);
        }
        else {
            transmit_file(comm, argv[4], TRANSMIT_MODE::NO_CRC);
        }
        close_comm(comm);
    }
    catch (char* message) {
        std::cerr << message << std::endl;
    }

    return 0;
}
