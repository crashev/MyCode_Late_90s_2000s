#include <zlib.h>
#include <string.h>

extern int decompress_inflate (Byte *compr, uLong comprLen, Byte *uncompr, 
				uLong uncomprLen);
