#ifndef COMMAND_H
#define COMMAND_H

#define RET_COMMAND_OK        (0)
#define RET_COMMAND_ERR_READ  (1)
#define RET_COMMAND_ERR_WRITE (2)

#if COMMAND_SRC
# define EXTERN 
#else
# define EXTERN extern
#endif


EXTERN int get_firmwareversion(SERHDL hdl, int myargc, char **myargv);
EXTERN int display_text(SERHDL hdl, int myargc, char **myargv);
EXTERN int store_text(SERHDL hdl, int myargc, char **myargv);
EXTERN int set_textspeed(SERHDL hdl, int myargc, char **myargv);
EXTERN int display_pattern(SERHDL hdl, int myargc, char **myargv);
EXTERN int store_pattern(SERHDL hdl, int myargc, char **myargv);
EXTERN int set_normalmode(SERHDL hdl, int myargc, char **myargv);
EXTERN int set_textmode(SERHDL hdl, int myargc, char **myargv);
EXTERN int set_patternmode(SERHDL hdl, int myargc, char **myargv);
EXTERN int exe_factoryreset(SERHDL hdl, int myargc, char **myargv);

#undef EXTERN

#endif
