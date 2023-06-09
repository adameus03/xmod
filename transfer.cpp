#include <windows.h>
#include <fstream>
#include <cstring>
#include "redundancy.h"
#include <iostream>

#define SOH 0x1   // 00000001
#define EOT 0x4   // 00000100
#define ACK 0x6   // 00000110
#define NAK 0x15  // 00010101
#define CAN 0x18  // 00011000
#define C 0x43    // 01000011

#define MAX_RECEIVE_FILE_SIZE 1000000

typedef unsigned char uchar;
typedef unsigned short ushort;

enum TRANSMIT_MODE {
    NO_CRC = SOH,
    CRC_16 = C
};

HANDLE get_comm(LPCSTR lp_file_name, DWORD baud_rate){
    HANDLE handle = CreateFile(lp_file_name, GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(handle==INVALID_HANDLE_VALUE){
        throw "Error occured while trying to create the COM handle";
    }
    DCB dcb_params = {0};
    dcb_params.DCBlength = sizeof(dcb_params);
    if(!GetCommState(handle, &dcb_params)){
        throw "Error occured while trying to load the COM state";
    }
    dcb_params.BaudRate = baud_rate;
    dcb_params.ByteSize = 0x8;
    dcb_params.StopBits = ONESTOPBIT;
    dcb_params.Parity = NOPARITY;
    if(!SetCommState(handle, &dcb_params)){
        throw "Error occured while trying to store the COM state";
    }

    COMMTIMEOUTS timeouts = {0};
    if(!GetCommTimeouts(handle, &timeouts)){
        throw "Error occured while trying to load COM timeouts";
    }
    timeouts.ReadTotalTimeoutConstant = 10000;
    timeouts.WriteTotalTimeoutConstant = 100;
    if(!SetCommTimeouts(handle, &timeouts)){
        throw "Error occured while trying to store COM timeouts";
    }

    //PurgeComm(handle, PURGE_RXCLEAR);

    return handle;
}

void close_comm(HANDLE handle){
    if(!CloseHandle(handle)){
        throw "Failure while trying to close the COM handle";
    };
}


void transmit_file(const HANDLE comm_handle, const char* source_path, TRANSMIT_MODE mode){
    std::ifstream f(source_path, std::ifstream::binary);
    std::filebuf* f_pbuf = f.rdbuf();
    size_t sbuff_size = f_pbuf->pubseekoff(0, f.end, f.in);
    uchar rem = sbuff_size % 128;
    sbuff_size += 128 - rem;
    f_pbuf->pubseekpos(0, f.in);
    uchar* sbuff = new uchar[sbuff_size];
    f_pbuf->sgetn((char*)sbuff, sbuff_size);
    for(uchar i=0x0; i<128-rem; i++){
        *(sbuff+sbuff_size-0x80+rem+i) = 0x0;
    }
    f.close();

    uchar* c = new uchar;
    *c = NAK;
    uchar* xmod_frame = new uchar[133]; //xmod frame ptr
    uchar* frame_head;
    uchar* sbuff_head = sbuff;
    uchar* sbuff_tail = sbuff+sbuff_size;
    uchar blk_ix = 0x0;
    uchar blk_ixc = 0xff;
    uchar* fb_written = new uchar;
    uchar fb_remaining;
    ReadFile(comm_handle, c, 0x1, NULL, NULL);

    while(sbuff_head < sbuff_tail){
        frame_head = xmod_frame;
        *frame_head++ = mode;
        *frame_head++ = blk_ix++;
        *frame_head++ = blk_ixc--;
        memcpy(frame_head, sbuff_head, 0x80);
        frame_head += 0x80;
        if(mode == TRANSMIT_MODE::NO_CRC){
            *frame_head = adt_checksum(xmod_frame, 0x83);
            fb_remaining = 0x84;
        }
        else {
            ushort ch = crc_16(xmod_frame, 0x83);
            *frame_head++ = ch >> 0x8;
            *frame_head = ch & 0xff; // is the mask neccessary?
            fb_remaining = 0x85;
        }
        *fb_written = 0x0;
        //fb_remaining = 0x84;
        frame_head = xmod_frame;
        std::cout << "Attempting to send XMODEM frame..." << std::endl;
        do {
            WriteFile(comm_handle, frame_head, fb_remaining, (LPDWORD)fb_written, NULL);
            fb_remaining -= *fb_written;
        }
        while(fb_remaining && (frame_head+=*fb_written));
        std::cout << "Successfully sent XMODEM frame!" << std::endl;
        ReadFile(comm_handle, c, 0x1, NULL, NULL);
        if(*c == ACK){
            std::cout << "Read ACK!" << std::endl;
            sbuff_head += 0x80;
        }
        else {
            std::cout << "Read NAK!" << std::endl;
        }
    }
    delete fb_written;

    do {
        *c = EOT;
        std::cout << "Attempting to send EOT" << std::endl;
        WriteFile(comm_handle, c, 0x1, NULL, NULL);
        std::cout << "Successfully sent EOT" << std::endl;
        ReadFile(comm_handle, c, 0x1, NULL, NULL);
    }
    while(*c != ACK);

    delete[] sbuff;
    delete[] xmod_frame;
    delete c;
}

void receive_file(const HANDLE comm_handle, const char* destination_path){
    uchar* dbuff = new uchar[MAX_RECEIVE_FILE_SIZE];
    uchar* dbuff_head = dbuff;
    uchar* cw = new uchar;
    uchar* cr = new uchar;
    uchar* xmod_frame = new uchar[133];
    uchar* frame_head;
    uchar* xmod_cksum = xmod_frame+131;
    uchar* xmod_read = new uchar;
    uchar xmod_remaining;
    uchar try_cnt = 0x0;
    *cw = NAK;


    while(true) {
        try_cnt = 0x0;
        do {
            if(try_cnt++==0x7) throw "Receiving file failed due to transmitter inactivity!";

            if(*cw == NAK) std::cout << "Preparing to write NAK" << std::endl;
            else if(*cw == ACK) std::cout << "Preparing to write ACK" << std::endl;

            WriteFile(comm_handle, cw, 0x1, NULL, NULL); //dodac timeout + max 1'

            if(*cw == NAK) std::cout << "Completed NAK write" << std::endl;
            else if(*cw == ACK) std::cout << "Completed ACK write" << std::endl;
        }
        while(!ReadFile(comm_handle, cr, 0x1, NULL, NULL));

        *xmod_frame = *cr;

        if(*cr == SOH){
            std::cout << "[Received SOH]" << std::endl;
            frame_head = xmod_frame+1;
            *xmod_read = 0x0;
            xmod_remaining = 0x83;
            do {
                ReadFile(comm_handle, frame_head, xmod_remaining, (LPDWORD)xmod_read, NULL);
                xmod_remaining -= *xmod_read;
                std::cout << "SOH handling...." << std::endl;
            }
            while(xmod_remaining && (frame_head+=*xmod_read));

            if(adt_checksum(xmod_frame, 0x83) == *xmod_cksum){
                *cw = ACK;
                memcpy(dbuff_head, xmod_frame+3, 0x80);
                dbuff_head += 0x80;
            }
            else {
                *cw = NAK;
            }
        }
        else if(*cr == C){
            std::cout << "[Received C]" << std::endl;
            frame_head = xmod_frame+1;
            *xmod_read = 0x0;
            xmod_remaining = 0x84;
            do {
                ReadFile(comm_handle, frame_head, xmod_remaining, (LPDWORD)xmod_read, NULL);
                xmod_remaining -= *xmod_read;
                std::cout << (int)(*xmod_read) << " read..." << std::endl;
                std::cout << (int)(xmod_remaining) << " remaining..." << std::endl;
            }
            while(xmod_remaining && (frame_head+=*xmod_read));

            ushort ch = crc_16(xmod_frame, 0x83);
            if((*xmod_cksum==(ch>>0x8)) && (*(xmod_cksum+1)==(ch&0xff))){ // again, is 0xff masking neccessary?
                std::cout << "Message correct [crc]" << std::endl;
                *cw = ACK;
                memcpy(dbuff_head, xmod_frame+3, 0x80);
                dbuff_head += 0x80;
            }
            else {
                std::cout << "Message incorrect [crc]" << std::endl;
                *cw = NAK;
            }
        }
        else if(*cr == EOT){
            *cw = ACK;
            std::cout << "[Received EOT]" << std::endl;
            std::cout << "Preparing to write ACK" << std::endl;
            WriteFile(comm_handle, cw, 0x1, NULL, NULL);
            std::cout << "Completed ACK write" << std::endl;
            break;
        }
        else {
            std::cout << "[Received unexpected signal]" << std::endl;
            PurgeComm(comm_handle, PURGE_RXCLEAR);
            *cw = NAK;
        }
    }
    delete[] xmod_frame;
    delete xmod_read;
    delete cr;
    delete cw;
    std::ofstream f(destination_path, std::ofstream::trunc | std::ofstream::binary);
    std::filebuf* f_pbuf = f.rdbuf();
    f_pbuf->sputn((char*)dbuff, (int)(dbuff_head-dbuff));
    f.close();
    delete[] dbuff;

}

