// 
// bool.c - performs some boolean operations
//
// must be initialized with a call to initbool()
// last changed april 13 2002

#include "checkers.h"
#include "bool.h"

#define USE_ASM // use assembler routines in LSB, MSB
				// only works with MS visual C

#ifndef USE_ASM
static char LSBarray[256];
static char MSBarray[256];
#endif
static char bitsinword[65536];
static int32 revword[65536];



int LSB(int32 x)
	{
	//-----------------------------------------------------------------------------------------------------
	// returns the position of the least significant bit in a 32-bit word x 
	// or -1 if not found, if x=0.
	// LSB uses asemmbler for higher speed.
	// for a portable version, use the code in /* */
	//-----------------------------------------------------------------------------------------------------

#ifdef USE_ASM

	__asm
		{
		mov	eax, -1				// -1 is the error value if no bit is set
		bsf	eax, dword ptr x    // value in eax is returned
		}

#else

	if(x&0x000000FF)
		return(LSBarray[x&0x000000FF]);
	if(x&0x0000FF00)
		return(LSBarray[(x>>8)&0x000000FF]+8);
	if(x&0x00FF0000)
		return(LSBarray[(x>>16)&0x000000FF]+16);
	if(x&0xFF000000)
		return(LSBarray[(x>>24)&0x000000FF]+24);
	return -1;
#endif

	}

int MSB(int32 x)
	{
	//-----------------------------------------------------------------------------------------------------
	// returns the position of the most significant bit in a 32-bit word x 
	// or -1 if not found, if x=0.
	// LSB uses assembler for high speed if USE_ASM is defined
	//-----------------------------------------------------------------------------------------------------

#ifdef USE_ASM
	
		__asm
		{
		mov	eax, -1				// -1 used as error value 
		bsr	eax, dword ptr x	// value returned in eax
		}
	
#else

	if(x&0xFF000000)
		return(MSBarray[(x>>24)&0xFF]+24);
	if(x&0x00FF0000)
		return(MSBarray[(x>>16)&0xFF]+16);
	if(x&0x0000FF00)
		return(MSBarray[(x>>8)&0xFF]+8);
	return(MSBarray[x&0xFF]);
	//if x==0 return MSBarray[0], that's ok, because it is set to -1 

#endif
	}


void initbool(void)
	{
	//-----------------------------------------------------------------------------------------------------
	// initialize the "bool.c" module
	// i.e. initialize lookup tables
	//-----------------------------------------------------------------------------------------------------

	int i,j;

#ifndef USE_ASM
	// initialize array for "LSB" 
	for(i=0;i<256;i++)
		{
		if(i&1) {LSBarray[i]=0;continue;}
		if(i&2) {LSBarray[i]=1;continue;}
		if(i&4) {LSBarray[i]=2;continue;}
		if(i&8) {LSBarray[i]=3;continue;}
		if(i&16) {LSBarray[i]=4;continue;}
		if(i&32) {LSBarray[i]=5;continue;}
		if(i&64) {LSBarray[i]=6;continue;}
		if(i&128) {LSBarray[i]=7;continue;}
		LSBarray[i]=-1;
		}

	// initialize array for "MSB" 
	for(i=0;i<256;i++)
		{
		if(i&128) {MSBarray[i]=7;continue;}
		if(i&64) {MSBarray[i]=6;continue;}
		if(i&32) {MSBarray[i]=5;continue;}
		if(i&16) {MSBarray[i]=4;continue;}
		if(i&8) {MSBarray[i]=3;continue;}
		if(i&4) {MSBarray[i]=2;continue;}
		if(i&2) {MSBarray[i]=1;continue;}
		if(i&1) {MSBarray[i]=0;continue;}
		MSBarray[i]=-1;
		}
#endif

	// initialize bitsinword, the number of bits in a word
	for(i=0;i<65536;i++)
		bitsinword[i]=recbitcount((int32)i);

	// initialize revword, the reverse of a word.
	for(i=0;i<65536;i++)
		{
		revword[i]=0;
		for(j=0;j<16;j++)
			{
			if(i&(1<<j))
				revword[i] +=1<<(15-j);
			}
		}
	}

int revert(int32 n)
	{
	//-----------------------------------------------------------------------------------------------------
	// reverses a 32-bit integer
	// needed to reverse a position when 
	//-----------------------------------------------------------------------------------------------------
	return (revword[hiword(n)] + (revword[loword(n)]<<16));
	}

int bitcount(int32 n)
	//-----------------------------------------------------------------------------------------------------
	// table-lookup bitcount 
	// returns the number of bits set in the 32-bit integer n 
	//-----------------------------------------------------------------------------------------------------
	{
	return (bitsinword[n&0x0000FFFF]+bitsinword[(n>>16)&0x0000FFFF]);
	}

int recbitcount(int32 n)
	{
	//-----------------------------------------------------------------------------------------------------
	// counts & returns the number of bits which are set in a 32-bit integer
	// slower than a table-based bitcount if many bits are
	// set. only used to make the table for the table-based bitcount on initialization
	//-----------------------------------------------------------------------------------------------------

	int r=0;
	while(n)
		{
		n=n&(n-1);
		r++;
		}
	return r;
	}
	
