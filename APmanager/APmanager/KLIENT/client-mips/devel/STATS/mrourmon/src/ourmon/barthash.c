/*
 * By Bart Massey, based on Knuth LCG.
 * Do an extra multiply to get the middle 32 bits
 * of the 64 bit product.
 */
static 
unsigned long __inline
barthash(int k) 
{
      /* 0x9e375365UL == 40499 * 65543 - both are prime;
         the result is ~0.68*(2^32) --Chuck Lever */

      return (0x9e37 * (k & 0xffff) + 0x5365 * (k >> 16));
}

