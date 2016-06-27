#ifndef PATTERN_H
#define PATTERN_H

#define RET_PATTERN_OK          (0)
#define RET_PATTERN_ERR_OPEN    (1)
#define RET_PATTERN_ERR_CLOSE   (2)
#define RET_PATTERN_ERR_READ    (3)

#if PATTERN_SRC
# define EXTERN 
#else
# define EXTERN extern
#endif

EXTERN int open_patternfile(char *path, FILE **handle);
EXTERN int read_patternfile(FILE *handle, unsigned char *linevalue);
EXTERN int close_patternfile(FILE *handle);

#undef EXTERN

#endif
