#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hw_types.h"

static unsigned char strtouc(char * buffer, char ** end, int base)
{
	unsigned char ret = 0;
	char c = *(buffer+2);
	*(buffer+2) = 0;
	ret = strtoul(buffer, end, base);
	*(buffer+2) = c;
	return ret;
}

int strToInt(char * data, char * type)
{
	if(strcmp(type, "dec") == 0)
		return strtol(data, NULL, 10);
	if(strcmp(type, "hex") == 0)
		return strtol(data, NULL, 16);
	if(strcmp(type, "bin") == 0)
		return strtol(data, NULL, 2);
	return -1;
}
char * intToStr(char * buffer, int number)
{
	sprintf(buffer, "%d", number);
	return buffer;
}

tBoolean streamToString(char * buffer, char * data, char * type)
{
	if(strcmp(type, "hex") == 0)
	{
		while(*data != 0)
		{
			sprintf(buffer, "%X", (unsigned int)(*data));
			buffer += 2;
		}
		return true;
	}
	if(strcmp(type, "string") == 0)
		if(buffer != data)
		{
			strcpy(buffer, ((char*)data));
			return true;
		}
	return false;
}

tBoolean stringToStream(char * buffer, char * data, char * type)
{
	if(strcmp(type, "hex") == 0)
	{
		while(*((char*)data) != 0 && *(((char*)data)+1) != 0)
		{
			*buffer++ = strtouc(data, (char**)&data, 16);
		}
		*buffer = 0;
		return true;
	}
	if(strcmp(type, "string") == 0)
		if(buffer != data)
		{
			strcpy(buffer, ((char*)data));
			return true;
		}
	return false;
}
