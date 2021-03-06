#ifndef SERIAL_H
#define SERIAL_H

#if LINUX
typedef int SERHDL;
#else
#  include <windows.h>
typedef HANDLE SERHDL;
#endif

#define RET_SERIAL_OK          (0)
#define RET_SERIAL_ERR_OPEN    (1)
#define RET_SERIAL_ERR_SETATTR (2)

#if SERIAL_SRC
# define EXTERN 
#else
# define EXTERN extern
#endif

EXTERN int open_serial(char *serialport, SERHDL *hdl);
EXTERN int close_serial(SERHDL hdl);
EXTERN int read_serial(SERHDL hdl, unsigned char *buf, int count);
EXTERN int write_serial(SERHDL hdl, unsigned char *buf, int count);


#undef EXTERN

#endif
