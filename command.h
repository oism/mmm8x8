#ifndef COMMAND_H
#define COMMAND_H

#define RET_COMMAND_OK (0)

#if COMMAND_SRC
# define EXTERN 
#else
# define EXTERN extern
#endif


EXTERN int get_firmwareversion(int fd, int myargc, char **myargv);
EXTERN int display_text(int fd, int myargc, char **myargv);
EXTERN int store_text(int fd, int myargc, char **myargv);
EXTERN int set_textspeed(int fd, int myargc, char **myargv);
EXTERN int display_pattern(int fd, int myargc, char **myargv);

#undef EXTERN

#endif
