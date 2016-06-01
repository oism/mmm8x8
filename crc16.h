#ifndef CRC16_H
#define CRC16_H

#define INITIAL_VALUE 0xffff

#if CRC16_SRC
# define EXTERN 
#else
# define EXTERN extern
#endif

EXTERN unsigned short calc_crc16(unsigned short initial, unsigned char value);

#undef EXTERN

#endif
