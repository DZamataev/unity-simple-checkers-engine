//
// index.c
//
// contains functions to convert a position into an index, or to convert an index into a position.
// these two functions are at the heart of the database builder
//
//
// general remarks on endgame database indexing:
//
// if you have k pieces which you can put on n squares, and they reside on x_0, x_1, ... ,x_k-1,
// where x_i is in the range 0....n-1, the "canonical" index for this configuration is given by:
//
// index = | x_0 | + | x_1 | + ... + | x_k-1 |, where | n | denotes the binomial coefficient 
//         |  1  |   |  2  |         |   k   |        | k |
//
// i use a straightforward position->index for black and white pieces, which reside on 
// 7 ranks or 28 squares, without checking for interference. therefore, the number of indexes
// my scheme produces is larger than necessary, with a few indexes resulting in bad positions.
//
// the black and white kings are added keeping the checker configuration in mind, i.e. counting
// not on which square a king is, but on which free square it is. 

// inverting position->index is easy if you know how position->index works:
// you need to know the number of pieces you are going to extract for a certain piece type.
// then, you know that the largest term in the index sum is | x_k-1 |
//                                                          |   k   |
// therefore, a sequential search beginning with | 31 |, | 30 |, etc. will find the right position.
//                                               | k  |  | k  |
// of piece number k, by the fact that the total index gets larger than one of these numbers. then
// it is time to subtract this number and continue the search until all pieces are resolved.
//
// in the case of checkers, this extraction immediately finds the position of the pieces on the
// board. in the case of kings, it only finds the number of the free square, so another step is
// needed, counting of free squares up to the number we found, to put the king on the board.
//
// i use the same scheme as schaeffer in chinook, with subdatabases which are classified according
// to the most advanced men. bmrank is a number between 0 and 6 and gives the rank on which the
// most advanced black man is. wmrank is similar, but seen from the white side. men interfere if
// bmrank+wmrank>6.
// the point of this is that the range of configurations shrinks in the database, and the amount
// it does can be subtracted from the man index before storing it, and added again after retrieving
// the index, as it is | 4*bmrank |
//                     |    bm    |
// there, that is all there is to indexing!


#include "checkers.h"
#include "index.h"
#include "bool.h"

extern int bicoef[33][33];
extern int bicoeftrans[33][33];
extern subdb subdatabase[MAXPIECE+1][MAXPIECE+1][MAXPIECE+1][MAXPIECE+1][7][7][2];

void positiontoindex(position *p, int32 *index)
	{
	//
	// computes an index for a given position
	//
	int bm, bk, wm, wk;
	
	int bmrank=0;
	int wmrank=0;

	int i;
	int32 x,y;
	int32 bmindex=0,bkindex=0,wmindex=0,wkindex=0;
	int32 bmrange=1, wmrange=1, bkrange=1;
	int32 posindex=0;
	
/*
          WHITE
   	   28  29  30  31           
	 24  25  26  27           
	   20  21  22  23          
	 16  17  18  19           
	   12  13  14  15          
	  8   9  10  11          
	    4   5   6   7           
	  0   1   2   3           
	      BLACK
*/

	// set bm, bk, wm, wk, and ranks
	bm = bitcount(p->bm);
	if(bm)
		bmrank = MSB(p->bm)/4;
	bk = bitcount(p->bk);
	wm = bitcount(p->wm);
	if(wm)
		wmrank = (31-LSB(p->wm))/4;
		
	wk = bitcount(p->wk);

	// first, we set the index for the black men:
	i=1;
	y=p->bm;
	while(y)
		{
		x=LSB(y);
		y=y^(1<<x);
		bmindex+=bicoef[x][i];
		i++;
		}

	// next, we set it for the white men, disregarding black men:
	i=1;
	y=p->wm;
	while(y)
		{
		x=MSB(y);
		y=y^(1<<x);
		x=31-x;
		wmindex+=bicoef[x][i];
		i++;
		}

	// then, black kings. this time, we include interfering black and white men.
	i=1;
	y=p->bk;
	while(y)
		{
		x=LSB(y);
		y=y^(1<<x);
		// next line is the count of men on squares 0....x-1, as x-1 of a 0000010000000 number is 0000000111111111
		x-=bitcount((p->bm|p->wm)&((1<<x)-1)); 
		bkindex+=bicoef[x][i];
		i++;
		}

	// last, white kings, with interfering other pieces
	i=1; 
	y=p->wk;
	while(y)
		{
		x=LSB(y);
		y^=(1<<x);
		x-=bitcount((p->bm|p->bk|p->wm) & ( (1<<x)-1 ) );
		wkindex+=bicoef[x][i];
		i++;
		}

	if(bm)
		bmrange = bicoef[4*(bmrank+1)][bm] - bicoef[4*bmrank][bm];
	if(wm)
		wmrange = bicoef[4*(wmrank+1)][wm] - bicoef[4*wmrank][wm];
	if(bk)
		bkrange = bicoef[32-bm-wm][bk];

	if(bmrank)
		bmindex -= bicoef[4*bmrank][bm];
	if(wmrank)
		wmindex -= bicoef[4*wmrank][wm];

	posindex = bmindex + wmindex*bmrange + bkindex*bmrange*wmrange + wkindex*bmrange*wmrange*bkrange;


	*index = posindex;	
	}


void indextoposition(int32 index, position *p, int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	// reverse the position to index thing

	int32 bmindex=0, bkindex=0, wmindex=0, wkindex=0;
	int32 bmrange=1, wmrange=1, bkrange=1;
	
	int multiplier;
	int32 square_one=1;
	int32 occupied;
	int i,j,k,f;
	int bkpos[MAXPIECE],wkpos[MAXPIECE];
	
	p->bm = 0;
	p->bk = 0;
	p->wm = 0;
	p->wk = 0;
	

	// extract the indexes for the piece types from the total index:
	// get multiplier for wkindex:
	if(bm)
		bmrange = bicoef[4*(bmrank+1)][bm] - bicoef[4*bmrank][bm];
		
	if(wm)
		wmrange = bicoef[4*(wmrank+1)][wm] - bicoef[4*wmrank][wm];
		
	if(bk)
		bkrange = bicoef[32-bm-wm][bk];
		
	multiplier = bmrange*wmrange*bkrange;
	wkindex = index / multiplier;
	index -= wkindex*multiplier;

	multiplier = bmrange*wmrange;
	bkindex = index / multiplier;
	index -= bkindex*multiplier;

	wmindex = index / bmrange;
	index -= wmindex*bmrange;

	bmindex = index;
	

	// add the rank index

	if(bm)
		bmindex += bicoef[4*bmrank][bm];
	if(wm)
		wmindex += bicoef[4*wmrank][wm];

	// now that we know the index numbers, we extract the pieces 
	// extract black men directly
	i=27;
	j=bm;
	while(j)
		{
		while(bicoeftrans[j][i]>bmindex)
			i--;
		bmindex-=bicoeftrans[j][i];
		p->bm |= square_one<<i;
		j--;
		}

	// extract white men directly
	i=27;
	j=wm;
	while(j)
		{
		while(bicoeftrans[j][i]>wmindex)
			i--;
		wmindex-=bicoeftrans[j][i];
		p->wm |= square_one<<(31-i);
		j--;
		}

	// extract positions of black kings
	i=31;
	for(j=bk;j>0;j--)
		{
		while(bicoeftrans[j][i]>bkindex)
			i--;
		bkindex-=bicoeftrans[j][i];
		bkpos[j-1]=i;
		}
	
	// extract positions of white kings
	i=31;
	for(j=wk;j>0;j--)
		{
		while(bicoeftrans[j][i]>wkindex)
			i--;
		wkindex-=bicoeftrans[j][i];
		wkpos[j-1]=i;
		}
	
	// now, put black kings on the board. we know: bkpos[0]...bkpos[bk-1] is ordered
	// with bkpos[0] being the smallest one, same goes for wkpos;

	occupied = p->bm|p->wm;
	k=0;
	f=0;
	for(i=0;i<32,k<bk;i++)
		{
		if(occupied & (square_one<<i))
			continue;
		if(bkpos[k] == f)
			{
			p->bk|=(square_one<<i);
			k++;
			}
		f++;
		}
	
	occupied = p->bm|p->wm|p->bk;
	k=0;
	f=0;
	for(i=0;i<32,k<wk;i++)
		{
		if(occupied & (square_one<<i))
			continue;
		if(wkpos[k] == f)
			{
			p->wk|=(square_one<<i);
			k++;
			}
		f++;
		}	
	}

