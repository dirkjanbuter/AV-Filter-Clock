#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

extern int filtercreate(int fps);
extern int filterstep(unsigned char *buffer, int w, int h, unsigned int color, char *text, int64_t framecount);


