#define CRC16_SRC 1
#include <crc16.h>
#undef CRC16_SRC

unsigned short calc_crc16(unsigned short initial, unsigned char value)
{
#define POLYNOM (0x8005)
  int i;
  unsigned short result;

  result = initial;
  for (i = 0; i < 8; i++)
  {
    if (((result & 0x8000) ^ (unsigned long) ((value & 0x80) << 8)) == 0x8000)
    {
      result = (result << 1) ^ POLYNOM;
    }
    else
    {
      result = (result << 1);
    }
  
    value = (value << 1);
  }

  return result;
}
