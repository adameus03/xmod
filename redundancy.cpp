typedef unsigned char uchar;
typedef unsigned short ushort;

#include <cstring>

#define CRC_POLYNOMIAL 0x1021

uchar adt_checksum(const uchar* data, uchar length){
    uchar sum = 0x0;
    uchar* h = (uchar*)data;
    for(uchar i=0x0; i<length; i++){
        sum += *h++;
    }
    return sum;
}

ushort crc_16(const uchar* data, uchar length){
    uchar* buffer = new uchar[length+0x2];
    memcpy(buffer, data, length);

    uchar xor_mode = 0x0;
    uchar xor_head;
    uchar s_uix;
    uchar s_bix;
    for(uchar uix=0x0; uix<length; uix++){

        for(uchar bix=0x0; bix<0x8; bix++){
            if(xor_mode){
                *(buffer+uix) ^= ((CRC_POLYNOMIAL<<xor_head)&0x80) >> bix;
                if(++xor_head == 0x10){
                    xor_mode = 0x0;
                    uix = s_uix;
                    bix = s_bix;
                }
            }
            else if(*(buffer+uix) & (0x80>>bix)){
                xor_mode = 0x1;
                xor_head = 0x0;
                s_uix = uix;
                s_bix = bix;
                --bix;
            }
        }
    }


    return ((ushort)(*(buffer+length))<<0x8) | (ushort)(*(buffer+length+1));
}
