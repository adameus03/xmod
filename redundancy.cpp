typedef unsigned char uchar;
typedef unsigned short ushort;

uchar adt_checksum(const uchar* data, uchar length){
    uchar sum = 0x0;
    uchar* h = (uchar*)data;
    for(uchar i=0x0; i<length; i++){
        sum += *h++;
    }
    return ~sum;
}

ushort crc_16(const uchar* data, uchar length){
    return 0x0;
}
