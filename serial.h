#ifndef SERIAL_H
#define SERIAL_H

#define RET_SERIAL_OK          (0)
#define RET_SERIAL_ERR_OPEN    (1)
#define RET_SERIAL_ERR_SETATTR (2)

#if SERIAL_SRC
# define EXTERN 
#else
# define EXTERN extern
#endif

EXTERN int open_serial(char *serialport, int *fd);
EXTERN int close_serial(int fd);
EXTERN int read_serial(int fd, unsigned char *buf, int count);
EXTERN int write_serial(int fd, unsigned char *buf, int count);


#undef EXTERN

#endif
