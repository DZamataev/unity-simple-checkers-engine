//
// lookup.c
//
// contains all the necessary code to look up database positions, except for boolean
// operations contained in bool.c / bool.h and for some definitions in checkers.h

#include "checkers.h"
#include "lookup.h"
#include "bool.h"
#include "index.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef SYS_WINDOWS
#include <windows.h>
#enlif SYS_MACOS
//#include "cakepp.h"
#endif

subdb subdatabase[MAXPIECE+1][MAXPIECE+1][MAXPIECE+1][MAXPIECE+1][7][7][2];
int bicoef[33][33];
int bicoeftrans[33][33];

// delete this line or remove extern if standalone
int32 meminuse;

// duplicated functions

int *currentdb_b, *currentdb_w;
int memorypanic(void)
{
    
    //-----------------------------------------------------------------------------------------------------
    // unloads all databases except currentdb_b and currentdb_w
    // can be called at any time to reduce memory load without disturbing calculation
    // however, if there is enough memory left, it should not be called excessively, as this is bad
    // for overall performance. ideally, this is only called when memory gets scarce.
    //-----------------------------------------------------------------------------------------------------
    
    int i,j,k,l,m,n;
    
    
    printf("\nmemory panic: %i bytes in use!", meminuse);
    meminuse = 0;
    //getch();
    for(i=0;i<=MAXPIECE;i++)
    {
        for(j=0;j<=MAXPIECE;j++)
        {
            for(k=0;k<=MAXPIECE;k++)
            {
                for(l=0;l<=MAXPIECE;l++)
                {
                    for(m=0;m<7;m++)
                    {
                        for(n=0;n<7;n++)
                        {
                            // black databases
                            if(subdatabase[i][j][k][l][m][n][0].database != NULL)
                            {
                                if(subdatabase[i][j][k][l][m][n][0].database != currentdb_b)
                                {
                                    printf("\nfreeing up %i%i%i%i.%i%i", i,j,k,l,m,n);
#ifdef SYS_WINDOWS
                                    VirtualFree(subdatabase[i][j][k][l][m][n][0].database , 0, MEM_RELEASE);
#elif SYS_MACOS
                                    free(subdatabase[i][j][k][l][m][n][0].database);
#endif
                                    subdatabase[i][j][k][l][m][n][0].database=NULL;
                                }
                                else
                                {
                                    meminuse += pad16(subdatabase[i][j][k][l][m][n][0].databasesize/4);
                                }
                            }
                            
                            // white databases
                            if(subdatabase[i][j][k][l][m][n][1].database != NULL)
                            {
                                if(subdatabase[i][j][k][l][m][n][1].database != currentdb_w)
                                {
                                    printf("\nfreeing up %i%i%i%i.%i%i", i,j,k,l,m,n);
#ifdef SYS_WINDOWS
                                    VirtualFree(subdatabase[i][j][k][l][m][n][1].database , 0, MEM_RELEASE);
#elif SYS_MACOS
                                    free(subdatabase[i][j][k][l][m][n][1].database);
#endif
                                    subdatabase[i][j][k][l][m][n][1].database=NULL;
                                }
                                else
                                {
                                    meminuse += pad16(subdatabase[i][j][k][l][m][n][1].databasesize/4);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    printf("\nmemory usage: %i",  meminuse);
    return 0;
}

int setdbname(int bm, int bk, int wm, int wk, int bmrank, int wmrank, int color, char *dbname)
{
    // SUICIDE CHECKERS: use "scdb%i..."
    // regular checkers: use "db%i..."
    if(color == BLACK)
        sprintf(dbname,"db%i%i%i%i-%i%ib.dat", bm,bk,wm,wk,bmrank,wmrank);
    else
        sprintf(dbname,"db%i%i%i%i-%i%iw.dat", bm,bk,wm,wk,bmrank,wmrank);
    return 1;
}

// ENDOF duplicated functions

int lookup(position *p, int color)
	{
	// returns WIN LOSS DRAW or UNKNOWN for a position in a database lookup.
	int32 index;
	int bm, bk, wm, wk, bmrank=0, wmrank=0;
	
	// next 2 lines for inlined version
	int32 bmindex=0,bkindex=0,wmindex=0,wkindex=0;
	int32 bmrange=1, /*wmrange=1,*/ bkrange=1, wkrange=1;
	
	int *database;
	int reverse = 0;
	int value=UNKNOWN;
	int32 memsize = 0;
	char dbname[64];
	
	position revpos, orgpos;
	FILE *fp;

	// set bm, bk, wm, wk, and ranks - bitcount is in bool.c
	bm = bool_bitcount(p->bm);
	if(bm)
		bmrank = MSB(p->bm)/4;
	bk = bool_bitcount(p->bk);
	wm = bool_bitcount(p->wm);
	if(wm)
		wmrank = (31-LSB(p->wm))/4;	
	wk = bool_bitcount(p->wk);
	
	//if one side has nothing, return appropriate value
	// regular checkers
    if(bm+bk==0)
        return color==BLACK?LOSS:WIN;
    if(wm+wk==0)
        return color==WHITE?LOSS:WIN;
	// reverse the signs above for suicide checkers!
	// SUICIDE CHECKERS
//    if(bm+bk==0)
//        return color==WHITE?LOSS:WIN;
//    if(wm+wk==0)
//        return color==BLACK?LOSS:WIN;

	
	
	// if this position is dominated by the "wrong" side, we have to
	// reverse it!
	if(wm+wk>bm+bk)
		reverse = 1;
	else 
		{
		if(wm+wk==bm+bk)
			{
			if(wk>bk)
				reverse = 1;
			if(bm==wm)
				{
				if(wmrank>bmrank)
					reverse = 1;
				if(wmrank==bmrank)
					{
					// this is a self-mirror:
					// only look at positions with black to move
					if(color==WHITE)
						{
						reverse = 1;
						}
					}
				}
			}
		}

	if (reverse)
		{
		// white is dominating this position, change colors
		orgpos = *p;
		revpos.bm = revert(p->wm);
		revpos.bk = revert(p->wk);
		revpos.wm = revert(p->bm);
		revpos.wk = revert(p->bk);
		*p = revpos;
		color^=1;
		
		reverse = bm;
		bm = wm;
		wm = reverse;
		
		reverse = bk;
		bk = wk;
		wk = reverse;
		
		reverse = bmrank;
		bmrank = wmrank;
		wmrank = reverse;
		
		reverse = 1;
		
		}

	database = subdatabase[bm][bk][wm][wk][bmrank][wmrank][color].database;
	
	// if database is not loaded, allocate memory for it
	if(database == NULL)
		{
		printf("\nallocating memory for %i%i%i%i.%i%i",bm, bk, wm, wk, bmrank, wmrank);
		if(color==BLACK)
			printf(" black to move");
		else
			printf(" white to move");
		memsize = pad16(subdatabase[bm][bk][wm][wk][bmrank][wmrank][color].databasesize/4);
//        database = malloc(memsize);
#ifdef SYS_WINDOWS
        database = VirtualAlloc(0, memsize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#elif SYS_MACOS
        database = malloc(memsize);
#endif
            

		meminuse += memsize;
		printf(" mem usage %iKB",meminuse/1024);

		if(database==NULL)
			{
			// we could not allocate memory for this database! help! - call memorypanic
                memorypanic();
                #ifdef SYS_WINDOWS
                database = VirtualAlloc(0, memsize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                #elif SYS_MACOS
                database = malloc(memsize);
                #endif
			if(database==NULL)
				{
				printf("\ncould not allocate memory despite memorypanic call");
				getchar();
				exit(0);
				}
			}
		subdatabase[bm][bk][wm][wk][bmrank][wmrank][color].database = database;
		// todo: load database from disk if it is not the current database.
		setdbname(bm, bk, wm, wk, bmrank, wmrank, color, dbname);
/*
		if(color==BLACK)
			sprintf(dbname,"raw\\db%i%i%i%i-%i%ib.dat", bm,bk,wm,wk,bmrank,wmrank);
		else
			sprintf(dbname,"raw\\db%i%i%i%i-%i%iw.dat", bm,bk,wm,wk,bmrank,wmrank);
*/
		fp = fopen(dbname,"rb");
		if(fp == NULL)
			{
			printf("\ncould not open database - %s", dbname);
			return -2;
			}
		else
			{
			fread(database, 1, memsize, fp);
			fclose(fp);
			}
		}
	
	positiontoindex(p,&index);

	value = getdatabasevalue(database, index);
	
	if(reverse)
		{
		// reset the position
		*p=orgpos;
		}
	return value;
	}

int getdatabasevalue(int *database, int index)
	// reads a value in the database
	{
	int32 value=UNKNOWN;
	//printf("%i ", index);
	// there are 16 entries per integer, therefore:
	value = database[index/16];

	value = (value >> (2*(index%16)) ) & 3;

	return value;
	}


///////////////////////////////////////////////////////////////////////////////////////////////////
// initialization stuff below



int64 getdatabasesize(int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	// returns the range of database indices for this database.
	// needs binomial coefficients in the array bicoef[][] = choose from n, k
	int64 dbsize = 1;

	// number of bm configurations:
	// there are bm black men subject to the constraint that one of them is on 
	// the rank bmrank

	if(bm)
		dbsize *= bicoef[4*(bmrank+1)][bm] - bicoef[4*bmrank][bm];
  
	if(wm)
		dbsize *= bicoef[4*(wmrank+1)][wm] - bicoef[4*wmrank][wm];

	// number of bk configurations
	if(bk)
		dbsize *= bicoef[32-bm-wm][bk];

	// number of wk configurations
	if(wk)
		dbsize *= bicoef[32-bm-wm-bk][wk];

	return dbsize;
	}


int initlookup(void)
	{
	// initializes the database records needed later for efficient access
	// of these databases
	// subdatabase[bm][bk][wm][wk][maxbm][maxwm]

	int i,j,k,l,m,n;

	// initialize binomial coefficients
	// bicoef[n][k] is supposed to be the number of ways you can choose k from n
	for(i=0;i<33;i++)
		{
		for(j=1;j<=i;j++)
			{
			// choose j from i:
			bicoef[i][j] = choose(i,j);
			}
		// define choosing 0 for simplicity 
		bicoef[i][0] = 1;
		}


	// choosing n from 0: bicoef = 0
	for(i=1;i<33;i++)
		bicoef[0][i]=0;

	// transposed bicoef array is faster sometimes.
	for(i=0;i<33;i++)
		{
		for(j=0;j<33;j++)
			bicoeftrans[i][j]=bicoef[j][i];
		}


	for(i=0;i<=MAXPIECE;i++)
		{
		for(j=0;j<=MAXPIECE;j++)
			{
			for(k=0;k<=MAXPIECE;k++)
				{
				for(l=0;l<=MAXPIECE;l++)
					{
					for(m=0;m<7;m++)
						{
						for(n=0;n<7;n++)
							{
							subdatabase[i][j][k][l][m][n][0].bm = i;
							subdatabase[i][j][k][l][m][n][0].bk = j;
							subdatabase[i][j][k][l][m][n][0].wm = k;
							subdatabase[i][j][k][l][m][n][0].wk = l;
							subdatabase[i][j][k][l][m][n][0].maxbm = m;
							subdatabase[i][j][k][l][m][n][0].maxwm = n;
							subdatabase[i][j][k][l][m][n][0].databasesize = getdatabasesize(i, j, k, l, m, n);
							subdatabase[i][j][k][l][m][n][1].bm = i;
							subdatabase[i][j][k][l][m][n][1].bk = j;
							subdatabase[i][j][k][l][m][n][1].wm = k;
							subdatabase[i][j][k][l][m][n][1].wk = l;
							subdatabase[i][j][k][l][m][n][1].maxbm = m;
							subdatabase[i][j][k][l][m][n][1].maxwm = n;
							subdatabase[i][j][k][l][m][n][1].databasesize = getdatabasesize(i, j, k, l, m, n);
							}
						}
					}
				}
			}
		}
	return 1;
	}


int32 choose(int n, int k)
	{
	// returns the binomial coefficient corresponding to choosing k from n, n!/(k! * (n-k)!)
	// this function is only used to fill in the array bicoef, which is used later for a 
	// fast lookup of these values.
	int64 result = 1;
	int64 i;

	i=k;
	while(i)
		{
		result *= (n-i+1);
		i--;
		}

	i=k;
	while(i)
		{
		result /=i;
		i--;
		}

	return (int32)result;
	}

