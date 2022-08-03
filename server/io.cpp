#include "io.h"

char* gbuf = 0;

void setgbuf(char* buf){
	static char* b = 0;
	if (buf == 0)
		gbuf = b;
	else
		gbuf = buf;
}	