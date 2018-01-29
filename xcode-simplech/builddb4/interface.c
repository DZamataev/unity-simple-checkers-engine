//
// interface.c - does the text routines for checkers
//

#include <stdio.h>
#include "checkers.h"
#include "interface.h"
#include "bool.h"


void movetonotation(position p, move m, char *str, int color)

	{
   /* make a notation out of a move */
   /* m is the move, str the string, color the side to move */

   int from, to;

   char c;

 /*

       WHITE
   	28  29  30  31           32  31  30  29
	 24  25  26  27           28  27  26  25
	   20  21  22  23           24  23  22  21
	 16  17  18  19           20  19  18  17
	   12  13  14  15           16  15  14  13
	  8   9  10  11           12  11  10   9
	    4   5   6   7            8   7   6   5
	  0   1   2   3            4   3   2   1
	      BLACK
*/

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
	static int square[32]={4,3,2,1,8,7,6,5,
   					 12,11,10,9,16,15,14,13,
                   20,19,18,17,24,23,22,21,
                   28,27,26,25,32,31,30,29}; /* maps bits to checkers notation */

   if(color==BLACK)
   	{
      if(m.wk|m.wm) c='x';
        	else c='-';                      /* capture or normal ? */
      from=(m.bm|m.bk)&(p.bm|p.bk);    /* bit set on square from */
      to=  (m.bm|m.bk)&(~(p.bm|p.bk));
      from=LSB(from);
      to=LSB(to);
      from=square[from];
      to=square[to];
      sprintf(str,"%2i%c%2i",from,c,to);
      }
   else
   	{
      if(m.bk|m.bm) c='x';
      else c='-';                      /* capture or normal ? */
      from=(m.wm|m.wk)&(p.wm|p.wk);    /* bit set on square from */
      to=  (m.wm|m.wk)&(~(p.wm|p.wk));
      from=LSB(from);
      to=LSB(to);
      from=square[from];
      to=square[to];
      sprintf(str,"%2i%c%2i",from,c,to);
      }
   return;
   }

void printboardtofile(position p, FILE *fp)
	{
	int i;
	int free=~(p.bm|p.bk|p.wm|p.wk);
	int b[32];
	char c[15]="-wb      WB";

	for(i=0;i<32;i++)
		{
		if((p.bm>>i)%2)
			b[i]=BLACK;
		if((p.bk>>i)%2)
			b[i]=BLACK|KING;
		if((p.wm>>i)%2)
			b[i]=WHITE;
		if((p.wk>>i)%2)
			b[i]=WHITE|KING;
		if((free>>i)%2)
			b[i]=0;
		}

	fprintf(fp,"\n\n");
	fprintf(fp,"\n %c %c %c %c",c[b[28]],c[b[29]],c[b[30]],c[b[31]]);
	fprintf(fp,"\n%c %c %c %c ",c[b[24]],c[b[25]],c[b[26]],c[b[27]]);
	fprintf(fp,"\n %c %c %c %c",c[b[20]],c[b[21]],c[b[22]],c[b[23]]);
	fprintf(fp,"\n%c %c %c %c ",c[b[16]],c[b[17]],c[b[18]],c[b[19]]);
	fprintf(fp,"\n %c %c %c %c",c[b[12]],c[b[13]],c[b[14]],c[b[15]]);
	fprintf(fp,"\n%c %c %c %c ",c[b[8]],c[b[9]],c[b[10]],c[b[11]]);
	fprintf(fp,"\n %c %c %c %c",c[b[4]],c[b[5]],c[b[6]],c[b[7]]);
	fprintf(fp,"\n%c %c %c %c ",c[b[0]],c[b[1]],c[b[2]],c[b[3]]);
	fflush(fp);
	}

void printboard(position p)
	{
   int i;
   int free=~(p.bm|p.bk|p.wm|p.wk);
   int b[32];
   char c[15]="bwBW-         ";
   printf("\n %X %X %X %X",p.bm, p.bk, p.wm, p.wk);
	
   for(i=0;i<32;i++)
   	{
      if((p.bm>>i)%2)
      	b[i]=BLACK;
      if((p.bk>>i)%2)
      	b[i]=BLACK|KING;
      if((p.wm>>i)%2)
      	b[i]=WHITE;
      if((p.wk>>i)%2)
      	b[i]=WHITE|KING;
      if((free>>i)%2)
      	b[i]=4;
      }


    printf("\n\n");
    printf("\n %c %c %c %c",c[b[28]],c[b[29]],c[b[30]],c[b[31]]);
    printf("\n%c %c %c %c ",c[b[24]],c[b[25]],c[b[26]],c[b[27]]);
    printf("\n %c %c %c %c",c[b[20]],c[b[21]],c[b[22]],c[b[23]]);
    printf("\n%c %c %c %c ",c[b[16]],c[b[17]],c[b[18]],c[b[19]]);
    printf("\n %c %c %c %c",c[b[12]],c[b[13]],c[b[14]],c[b[15]]);
    printf("\n%c %c %c %c ",c[b[8]],c[b[9]],c[b[10]],c[b[11]]);
    printf("\n %c %c %c %c",c[b[4]],c[b[5]],c[b[6]],c[b[7]]);
    printf("\n%c %c %c %c ",c[b[0]],c[b[1]],c[b[2]],c[b[3]]);
   }