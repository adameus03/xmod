#ifndef REDUNDANCY_H_INCLUDED
#define REDUNDANCY_H_INCLUDED

typedef unsigned char uchar;

unsigned char adt_checksum(const uchar* data, uchar length);

unsigned short crc_16(const uchar* data, uchar length);

#endif // REDUNDANCY_H_INCLUDED
