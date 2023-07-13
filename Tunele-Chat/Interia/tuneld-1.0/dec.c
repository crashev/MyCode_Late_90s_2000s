#include "dec.h"

char *
check_err (int err)
{
  static char showe[100];
  switch (err)
    {
    case -1:
      strcpy (showe, "ERRNO");
      break;
    case -2:
      strcpy (showe, "STREAM ERROR");
      break;
    case -3:
      strcpy (showe, "DATA ERROR");
      break;
    case -4:
      strcpy (showe, "MEM ERROR");
      break;
    case -5:
      strcpy (showe, "BUF ERROR");
      break;
    case -6:
      strcpy (showe, "VERSIOn ERROR");
      break;
    default:
      strcpy (showe, "OK");
      break;
    }
  return (char *) showe;
}



int
decompress_inflate (compr, comprLen, uncompr, uncomprLen)
     Byte *compr, *uncompr;
     uLong comprLen, uncomprLen;
{
  int err;
//  int x = 0;
  z_stream d_stream;		/* decompression stream */

  strcpy ((char *) uncompr, "garbage");

  d_stream.zalloc = (alloc_func) 0;
  d_stream.zfree = (free_func) 0;
  d_stream.opaque = (voidpf) 0;

  d_stream.next_in = compr;
  d_stream.avail_in = 0;
  d_stream.next_out = uncompr;

  err = inflateInit (&d_stream);

  while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen)
    {
      d_stream.avail_in = d_stream.avail_out = 1;	/* force small buffers */
      err = inflate (&d_stream, Z_NO_FLUSH);
      if (err == Z_STREAM_END)
	break;
      if (err < 0)
	{
//	  printf ("Inflate %s \n", check_err (err));
	  return -1;
	}

    }

  err = inflateEnd (&d_stream);
  return 0;
}
