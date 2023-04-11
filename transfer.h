#ifndef TRANSFER_H_INCLUDED
#define TRANSFER_H_INCLUDED

#include <windows.h>
#include <fstream>
#include <cstring>
#include <iostream>

enum TRANSMIT_MODE {
    NO_CRC = 0x1,
    CRC_16 = 0x43
};

HANDLE get_comm(LPCSTR lp_file_name, DWORD baud_rate);

void close_comm(HANDLE handle);

void transmit_file(const HANDLE comm_handle, const char* source_path, TRANSMIT_MODE mode);

void receive_file(const HANDLE comm_handle, const char* destination_path);

#endif // TRANSFER_H_INCLUDED
