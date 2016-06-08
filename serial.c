#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <fcntl.h>

#define SERIAL_SRC 1
#include <serial.h>
#undef SERIAL_SRC

int open_serial(char *serialport, int *fd)
{
  int rc;
  struct termios options;

  if ((*fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY)) == -1)
  {
    rc = RET_SERIAL_ERR_OPEN;
    goto EXIT;
  }

  /* now set 38400,8,N,1 */
  /* get the current settings of the serial port */
  tcgetattr(*fd, &options);

  /* set the read and write speed to 38400 BAUD */
  cfsetispeed(&options, B38400);
  cfsetospeed(&options, B38400);

  /* disable the parity bit */
  options.c_cflag &= ~PARENB;

  /* only one stop bit */
  options.c_cflag &= ~CSTOPB;

  /* 8-bits */
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;

  /* enable the receiver and set local mode */
  options.c_cflag |= (CLOCAL | CREAD);

  /* choosing raw input */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  /* choosing raw output */
  options.c_oflag &= ~OPOST;
 
  /* now set the settings */
  if(tcsetattr(*fd, TCSANOW, &options) == -1)
  {
    close(*fd);
    rc = RET_SERIAL_ERR_SETATTR;
    goto EXIT;
  }

  rc = RET_SERIAL_OK;

EXIT:
  return rc;
}


int close_serial(int fd)
{
  close(fd);

  return RET_SERIAL_OK;
}
