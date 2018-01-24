//
//  simplech.c
//  simplech
//
//  Created by Denis Zamataev on 19/01/2018.
//  Copyright © 2018 org. All rights reserved.
//

#include "simplech.h"

/*______________________________________________________________________________
 
 ----------> name: simple checkers with enhancements
 ----------> author: martin fierz
 ----------> purpose: platform independent checkers engine
 ----------> version: 1.14
 ----------> date: 22nd september 2002
 ----------> description: simplech.c contains a simple but fast checkers engine
 and some routines to interface to checkerboard. simplech.c contains three
 main parts: interface, search and move generation. these parts are
 separated in the code.
 
 board representation: the standard checkers notation is
 
 (white)
 32  31  30  29
 28  27  26  25
 24  23  22  21
 20  19  18  17
 16  15  14  13
 12  11  10   9
 8   7   6   5
 4   3   2   1
 (black)
 
 the internal representation of the board is different, it is a
 array of int with length 46, the checkers board is numbered
 like this:
 
 (white)
 37  38  39  40
 32  33  34  35
 28  29  30  31
 23  24  25  26
 19  20  21  22
 14  15  16  17
 10  11  12  13
 5   6   7   8
 (black)
 
 let's say, you would like to teach the program that it is
 important to keep a back rank guard. you can for instance
 add the following (not very sophisticated) code for this:
 
 if(b[6] & (BLACK|MAN)) eval++;
 if(b[8] & (BLACK|MAN)) eval++;
 if(b[37] & (WHITE|MAN)) eval--;
 if(b[39] & (WHITE|MAN)) eval--;
 
 the evaluation function is seen from the point of view of the
 black player, so you increase the value v if you think the
 position is good for black.
 
 
 have fun!
 
 questions, comments, suggestions to:
 
 Martin Fierz
 checkers@fierz.ch
 
 ___________________________________*/

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif
 
 /*----------> includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
/*----------> definitions */
#define OCCUPIED 0
#define WHITE 1
#define BLACK 2
#define MAN 4
#define KING 8
#define FREE 16
#define CHANGECOLOR 3
#define MAXDEPTH 99
#define MAXMOVES 28

/*----------> compile options  */
#undef MUTE
#undef VERBOSE
#define STATISTICS


/* return values */
#define DRAW 0
#define WIN 1
#define LOSS 2
#define UNKNOWN 3

/*----------> structure definitions  */
struct move2
{
    short n;
    int m[8];
};
struct coor             /* coordinate structure for board coordinates */
{
    int x;
    int y;
};

struct CBmove                /* all the information you need about a move */
{
    int ismove;          /* kind of superfluous: is 0 if the move is not a valid move */
    int newpiece;        /* what type of piece appears on to */
    int oldpiece;        /* what disappears on from */
    struct coor from,to; /* coordinates of the piece - in 8x8 notation!*/
    struct coor path[12]; /* intermediate path coordinates of the moving piece */
    struct coor del[12]; /* squares whose pieces are deleted after the move */
    int delpiece[12];    /* what is on these squares */
} GCBmove;
/*----------> function prototypes  */

/* sample functions */
MONExternC int ** EXPORT_API fillArray(int size) {
    int i = 0, j = 0;
    int ** array = (int**) calloc(size, sizeof(int*));
    for(i = 0; i < size; i++) {
        array[i] = (int*) calloc(size, sizeof(int));
        for(j = 0; j < size; j++) {
            array[i][j] = i * size + j;
        }
    }
    return array;
}

MONExternC int EXPORT_API sumArray(int b[8]) {
    int sum = 0;
    for (int i = 0; i < 8; i++) {
        sum = b[i]+sum;
    }
    return sum;
}

/*----------> part I: interface to CheckerBoard: CheckerBoard requires that
 at getmove and enginename are present in the dll. the
 functions help, options and about are optional. if you
 do not provide them, CheckerBoard will display a
 MessageBox stating that this option is in fact not an option*/


/* required functions */
//int getmove(int b[8][8],int color, double maxtime, char str[255], int *playnow, int info, int unused, struct CBmove *move);
//void movetonotation(struct move2 move,char str[80]);


/*----------> part II: search */
int  checkers(int b[46],int color, double maxtime, char *str);
int  alphabeta(int b[46],int depth, int alpha, int beta, int color);
int  firstalphabeta(int b[46],int depth, int alpha, int beta, int color,struct move2 *best);
void domove(int b[46],struct move2 move);
void undomove(int b[46],struct move2 move);
int  evaluation(int b[46], int color);

/*----------> part III: move generation */
int  generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color);
int  generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color);
void blackmancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
void blackkingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
void whitemancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
void whitekingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
int  testcapture(int b[46], int color);




/*----------> globals  */
#ifdef STATISTICS
int alphabetas,generatemovelists,evaluations,generatecapturelists,testcaptures;
#endif
int value[17]={0,0,0,0,0,1,256,0,0,16,4096,0,0,0,0,0,0};
int *play;


/*-------------- PART 1: dll stuff -------------------------------------------*/

//BOOL WINAPI DllEntryPoint (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
//{
//    /* in a dll you used to have LibMain instead of WinMain in windows programs, or main
//     in normal C programs
//     win32 replaces LibMain with DllEntryPoint.*/
//
//    switch (dwReason)
//    {
//        case DLL_PROCESS_ATTACH:
//            /* dll loaded. put initializations here! */
//            break;
//        case DLL_PROCESS_DETACH:
//            /* program is unloading dll. put clean up here! */
//            break;
//        case DLL_THREAD_ATTACH:
//            break;
//        case DLL_THREAD_DETACH:
//            break;
//        default:
//            break;
//    }
//    return TRUE;
//}

MONExternC int EXPORT_API enginecommand(char str[256], char reply[256])
{
    // answer to commands sent by CheckerBoard.
    // Simple Checkers does not answer to some of the commands,
    // eg it has no engine options.
    
    char command[256],param1[256],param2[256];
    
    sscanf(str,"%s %s %s",command,param1,param2);
    
    // check for command keywords
    
    if(strcmp(command,"name")==0)
    {
        sprintf(reply,"Simple Checkers 1.14");
        return 1;
    }
    
    if(strcmp(command,"about")==0)
    {
        sprintf(reply,"Simple Checkers 1.14\n\n2002 by Martin Fierz");
        return 1;
    }
    
    if(strcmp(command,"help")==0)
    {
        sprintf(reply,"simplechhelp.htm");
        return 1;
    }
    
    if(strcmp(command,"set")==0)
    {
        if(strcmp(param1,"hashsize")==0)
        {
            sprintf(reply,"?");
            return 0;
        }
        if(strcmp(param1,"book")==0)
        {
            sprintf(reply,"?");
            return 0;
        }
    }
    
    if(strcmp(command,"get")==0)
    {
        if(strcmp(param1,"hashsize")==0)
        {
            sprintf(reply,"?");
            return 0;
        }
        if(strcmp(param1,"book")==0)
        {
            sprintf(reply,"?");
            return 0;
        }
        if(strcmp(param1,"protocolversion")==0)
        {
            sprintf(reply,"2");
            return 1;
        }
        if(strcmp(param1,"gametype")==0)
        {
            sprintf(reply,"21");
            return 1;
        }
    }
    sprintf(reply,"?");
    return 0;
}



//MONExternC int EXPORT_API getmove(int b[8][8],int color, double maxtime, char str[255], int *playnow, int info, int unused, struct CBmove *move)
MONExternC int EXPORT_API getmove(int b[33],int color, double maxtime)
{
    int playnowvalue = 0;
    int* playnow = &playnowvalue;
    char str[256] = {0};
    /* getmove is what checkerboard calls. you get 6 parameters:
     b[8][8]     is the current position. the values in the array are determined by
     the #defined values of BLACK, WHITE, KING, MAN. a black king for
     instance is represented by BLACK|KING.
     color        is the side to make a move. BLACK or WHITE.
     maxtime    is the time your program should use to make a move. this is
     what you specify as level in checkerboard. so if you exceed
     this time it's not too bad - just don't exceed it too much...
     str        is a pointer to the output string of the checkerboard status bar.
     you can use sprintf(str,"information"); to print any information you
     want into the status bar.
     *playnow    is a pointer to the playnow variable of checkerboard. if the user
     would like your engine to play immediately, this value is nonzero,
     else zero. you should respond to a nonzero value of *playnow by
     interrupting your search IMMEDIATELY.
     struct CBmove *move
     is unused here. this parameter would allow engines playing different
     versions of checkers to return a move to CB. for engines playing
     english checkers this is not necessary.
     */
    int i;
    int value;
    int board[46];
    
    /* initialize board */
    for(i=0;i<46;i++)
        board[i]=OCCUPIED;
    for(i=5;i<=40;i++)
        board[i]=FREE;
    /*    (white)
     37  38  39  40
     32  33  34  35
     28  29  30  31
     23  24  25  26
     19  20  21  22
     14  15  16  17
     10  11  12  13
     5   6   7   8
     (black)   */
    board[5]=b[4];board[6]=b[3];board[7]=b[2];board[8]=b[1];
    board[10]=b[8];board[11]=b[7];board[12]=b[6];board[13]=b[5];
    board[14]=b[12];board[15]=b[11];board[16]=b[10];board[17]=b[9];
    board[19]=b[16];board[20]=b[15];board[21]=b[14];board[22]=b[13];
    board[23]=b[20];board[24]=b[19];board[25]=b[18];board[26]=b[17];
    board[28]=b[24];board[29]=b[23];board[30]=b[22];board[31]=b[21];
    board[32]=b[28];board[33]=b[27];board[34]=b[26];board[35]=b[25];
    board[37]=b[32];board[38]=b[31];board[39]=b[30];board[40]=b[29];
    for(i=5;i<=40;i++) {
        if(board[i]==0) {
            board[i]=FREE;
        }
    }
    for(i=9;i<=36;i+=9) {
        board[i]=OCCUPIED;
    }
    play=playnow;
    
    value=checkers(board,color,maxtime,str);
    
    // not needed if you are ok with FREE beaten cells
//    for(i=5;i<=40;i++) {
//        if(board[i]==FREE) {
//            board[i]=0;
//        }
//    }
    
    /* return the board */
    b[4]=board[5];b[3]=board[6];b[2]=board[7];b[1]=board[8];
    b[8]=board[10];b[7]=board[11];b[6]=board[12];b[5]=board[13];
    b[12]=board[14];b[11]=board[15];b[10]=board[16];b[9]=board[17];
    b[16]=board[19];b[15]=board[20];b[14]=board[21];b[13]=board[22];
    b[20]=board[23];b[19]=board[24];b[18]=board[25];b[17]=board[26];
    b[24]=board[28];b[23]=board[29];b[22]=board[30];b[21]=board[31];
    b[28]=board[32];b[27]=board[33];b[26]=board[34];b[25]=board[35];
    b[32]=board[37];b[31]=board[38];b[30]=board[39];b[29]=board[40];
    if(color==BLACK)
    {
        if(value>4000) return WIN;
        if(value<-4000) return LOSS;
    }
    if(color==WHITE)
    {
        if(value>4000) return LOSS;
        if(value<-4000) return WIN;
    }
    return UNKNOWN;
}


MONExternC void EXPORT_API movetonotation(struct move2 move,char str[80])
{
    int j,from,to;
    char c;
    
    from=move.m[0]%256;
    to=move.m[1]%256;
    from=from-(from/9);
    to=to-(to/9);
    from-=5;
    to-=5;
    j=from%4;from-=j;j=3-j;from+=j;
    j=to%4;to-=j;j=3-j;to+=j;
    from++;
    to++;
    c='-';
    if(move.n>2) c='x';
    sprintf(str,"%2i%c%2i",from,c,to);
}




/*-------------- PART II: SEARCH ---------------------------------------------*/


int  checkers(int b[46],int color, double maxtime, char *str)
/*----------> purpose: entry point to checkers. find a move on board b for color
 ---------->          in the time specified by maxtime, write the best move in
 ---------->          board, returns information on the search in str
 ----------> returns 1 if a move is found & executed, 0, if there is no legal
 ----------> move in this position.
 ----------> version: 1.1
 ----------> date: 9th october 98 */
{
    int i,numberofmoves;
    double start;
    int eval;
    struct move2 best,lastbest,movelist[MAXMOVES];
    char str2[255];
#ifdef STATISTICS
    alphabetas=0;
    generatemovelists=0;
    generatecapturelists=0;
    evaluations=0;
#endif
    /*--------> check if there is only one move */
    numberofmoves=generatecapturelist(b,movelist,color);
    if(numberofmoves==1) {domove(b,movelist[0]);sprintf(str,"forced capture");return(1);}
    else
    {
        numberofmoves=generatemovelist(b,movelist,color);
        if(numberofmoves==1) {domove(b,movelist[0]);sprintf(str,"only move");return(1);}
        if(numberofmoves==0) {sprintf(str,"no legal moves in this position");return(0);}
    }
    start=clock();
    
    eval=firstalphabeta(b,1,-10000,10000,color,&best);
    
    for(i=2;(i<=MAXDEPTH) && ( (clock()-start)/CLOCKS_PER_SEC < maxtime );i++)
    {
        lastbest=best;
        eval=firstalphabeta(b,i,-10000,10000,color,&best);
        movetonotation(best,str2);
#ifndef MUTE
        sprintf(str,"best:%s time %2.2fs, depth %2i, value %4i",str2,(clock()-start)/CLOCKS_PER_SEC,i,eval);
#ifdef STATISTICS
        sprintf(str2,"  nodes %i, gms %i, gcs %i, evals %i",
                alphabetas,generatemovelists,generatecapturelists,
                evaluations);
        strcat(str,str2);
#endif
#endif
        if(*play) break;
        if(eval==5000) break;
        if(eval==-5000) break;
    }
    i--;
    
    if(*play)
        movetonotation(lastbest,str2);
    else
        movetonotation(best,str2);
    
    sprintf(str,"best:%s time %2.2f, depth %2i, value %4i  nodes %i, gms %i, gcs %i, evals %i",str2,(clock()-start)/CLOCKS_PER_SEC,i,eval,alphabetas,generatemovelists,generatecapturelists,evaluations);
    
    if(*play)
        domove(b,lastbest);
    else
        domove(b,best);
    return eval;
}

int firstalphabeta(int b[46], int depth, int alpha, int beta, int color, struct move2 *best)
/*----------> purpose: search the game tree and find the best move.
 ----------> version: 1.0
 ----------> date: 25th october 97 */
{
    int i;
    int value;
    int numberofmoves;
    int capture;
    struct move2 movelist[MAXMOVES];
    
#ifdef STATISTICS
    alphabetas++;
#endif
    if (*play) return 0;
    
    /*----------> test if captures are possible */
    capture=testcapture(b,color);
    
    /*----------> recursion termination if no captures and depth=0*/
    if(depth==0)
    {
        if(capture==0)
            return(evaluation(b,color));
        else
            depth=1;
    }
    
    /*----------> generate all possible moves in the position */
    if(capture==0)
    {
        numberofmoves=generatemovelist(b,movelist,color);
        /*----------> if there are no possible moves, we lose: */
        if(numberofmoves==0)  {if (color==BLACK) return(-5000); else return(5000);}
    }
    else
        numberofmoves=generatecapturelist(b,movelist,color);
    
    /*----------> for all moves: execute the move, search tree, undo move. */
    for(i=0;i<numberofmoves;i++)
    {
        domove(b,movelist[i]);
        
        value=alphabeta(b,depth-1,alpha,beta,(color^CHANGECOLOR));
        
        undomove(b,movelist[i]);
        if(color == BLACK)
        {
            if(value>=beta) return(value);
            if(value>alpha) {alpha=value;*best=movelist[i];}
        }
        if(color == WHITE)
        {
            if(value<=alpha) return(value);
            if(value<beta)   {beta=value;*best=movelist[i];}
        }
    }
    if(color == BLACK)
        return(alpha);
    return(beta);
}

int alphabeta(int b[46], int depth, int alpha, int beta, int color)
/*----------> purpose: search the game tree and find the best move.
 ----------> version: 1.0
 ----------> date: 24th october 97 */
{
    int i;
    int value;
    int capture;
    int numberofmoves;
    struct move2 movelist[MAXMOVES];
    
#ifdef STATISTICS
    alphabetas++;
#endif
    if (*play) return 0;
    /*----------> test if captures are possible */
    capture=testcapture(b,color);
    
    /*----------> recursion termination if no captures and depth=0*/
    if(depth==0)
    {
        if(capture==0)
            return(evaluation(b,color));
        else
            depth=1;
    }
    
    /*----------> generate all possible moves in the position */
    if(capture==0)
    {
        numberofmoves=generatemovelist(b,movelist,color);
        /*----------> if there are no possible moves, we lose: */
        if(numberofmoves==0)  {if (color==BLACK) return(-5000); else return(5000);}
    }
    else
        numberofmoves=generatecapturelist(b,movelist,color);
    
    /*----------> for all moves: execute the move, search tree, undo move. */
    for(i=0;i<numberofmoves;i++)
    {
        domove(b,movelist[i]);
        
        value=alphabeta(b,depth-1,alpha,beta,color^CHANGECOLOR);
        
        undomove(b,movelist[i]);
        
        if(color == BLACK)
        {
            if(value>=beta) return(value);
            if(value>alpha) alpha=value;
        }
        if(color == WHITE)
        {
            if(value<=alpha) return(value);
            if(value<beta)   beta=value;
        }
    }
    if(color == BLACK)
        return(alpha);
    return(beta);
}

void domove(int b[46],struct move2 move)
/*----------> purpose: execute move on board
 ----------> version: 1.1
 ----------> date: 25th october 97 */
{
    int square,after;
    int i;
    
    for(i=0;i<move.n;i++)
    {
        square=(move.m[i] % 256);
        after=((move.m[i]>>16) % 256);
        b[square]=after;
    }
}

void undomove(int b[46],struct move2 move)
/*----------> purpose:
 ----------> version: 1.1
 ----------> date: 25th october 97 */
{
    int square,before;
    int i;
    
    for(i=0;i<move.n;i++)
    {
        square=(move.m[i] % 256);
        before=((move.m[i]>>8) % 256);
        b[square]=before;
    }
}

int evaluation(int b[46], int color)
/*----------> purpose:
 ----------> version: 1.1
 ----------> date: 18th april 98 */
{
    int i,j;
    int eval;
    int v1,v2;
    int nbm,nbk,nwm,nwk;
    int nbmc=0,nbkc=0,nwmc=0,nwkc=0;
    int nbme=0,nbke=0,nwme=0,nwke=0;
    int code=0;
    static int value[17]={0,0,0,0,0,1,256,0,0,16,4096,0,0,0,0,0,0};
    static int edge[14]={5,6,7,8,13,14,22,23,31,32,37,38,39,40};
    static int center[8]={15,16,20,21,24,25,29,30};
    static int row[41]={0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,0,3,3,3,3,4,4,4,4,0,5,5,5,5,6,6,6,6,0,7,7,7,7};
    static int safeedge[4]={8,13,32,37};
    
    int tempo=0;
    int nm,nk;
    
    const int turn=2;   //color to move gets +turn
    const int brv=3;    //multiplier for back rank
    const int kcv=5;    //multiplier for kings in center
    const int mcv=1;    //multiplier for men in center
    
    const int mev=1;    //multiplier for men on edge
    const int kev=5;    //multiplier for kings on edge
    const int cramp=5;  //multiplier for cramp
    
    const int opening=-2; // multipliers for tempo
    const int midgame=-1;
    const int endgame=2;
    const int intactdoublecorner=3;
    
    
    int backrank;
    
    int stonesinsystem=0;
    
#ifdef STATISTICS
    evaluations++;
#endif
    
    
    for(i=5;i<=40;i++)
        code+=value[b[i]];
    
    nwm = code % 16;
    nwk = (code>>4) % 16;
    nbm = (code>>8) % 16;
    nbk = (code>>12) % 16;
    
    
    v1=100*nbm+130*nbk;
    v2=100*nwm+130*nwk;
    
    eval=v1-v2;                       /*material values*/
    eval+=(250*(v1-v2))/(v1+v2);      /*favor exchanges if in material plus*/
    
    nm=nbm+nwm;
    nk=nbk+nwk;
    /*--------- fine evaluation below -------------*/
    
    if(color == BLACK) eval+=turn;
    else eval-=turn;
    /*    (white)
     37  38  39  40
     32  33  34  35
     28  29  30  31
     23  24  25  26
     19  20  21  22
     14  15  16  17
     10  11  12  13
     5   6   7   8
     (black)   */
    /* cramp */
    if(b[23]==(BLACK|MAN) && b[28]==(WHITE|MAN)) eval+=cramp;
    if(b[22]==(WHITE|MAN) && b[17]==(BLACK|MAN)) eval-=cramp;
    
    /* back rank guard */
    
    code=0;
    if(b[5] & MAN) code++;
    if(b[6] & MAN) code+=2;
    if(b[7] & MAN) code+=4;
    if(b[8] & MAN) code+=8;
    switch (code)
    {
        case 0: code=0;break;
        case 1: code=-1;break;
        case 2: code=1;break;
        case 3: code=0;break;
        case 4: code=1;break;
        case 5: code=1;break;
        case 6: code=2;break;
        case 7: code=1;break;
        case 8: code=1;break;
        case 9: code=0;break;
        case 10: code=7;break;
        case 11: code=4;break;
        case 12: code=2;break;
        case 13: code=2;break;
        case 14: code=9;break;
        case 15: code=8;break;
    }
    backrank=code;
    
    
    code=0;
    if(b[37] & MAN) code+=8;
    if(b[38] & MAN) code+=4;
    if(b[39] & MAN) code+=2;
    if(b[40] & MAN) code++;
    switch (code)
    {
        case 0: code=0;break;
        case 1: code=-1;break;
        case 2: code=1;break;
        case 3: code=0;break;
        case 4: code=1;break;
        case 5: code=1;break;
        case 6: code=2;break;
        case 7: code=1;break;
        case 8: code=1;break;
        case 9: code=0;break;
        case 10: code=7;break;
        case 11: code=4;break;
        case 12: code=2;break;
        case 13: code=2;break;
        case 14: code=9;break;
        case 15: code=8;break;
    }
    backrank-=code;
    eval+=brv*backrank;
    
    
    /* intact double corner */
    if(b[8]==(BLACK|MAN))
    {
        if(b[12]==(BLACK|MAN) || b[13]==(BLACK|MAN))
            eval+=intactdoublecorner;
    }
    
    if(b[37]==(WHITE|MAN))
    {
        if(b[32]==(WHITE|MAN) || b[33]==(WHITE|MAN))
            eval-=intactdoublecorner;
    }
    /*    (white)
     37  38  39  40
     32  33  34  35
     28  29  30  31
     23  24  25  26
     19  20  21  22
     14  15  16  17
     10  11  12  13
     5   6   7   8
     (black)   */
    
    /* center control */
    for(i=0;i<8;i++)
    {
        if(b[center[i]] != FREE)
        {
            if(b[center[i]] == (BLACK|MAN)) nbmc++;
            if(b[center[i]] == (BLACK|KING)) nbkc++;
            if(b[center[i]] == (WHITE|MAN)) nwmc++;
            if(b[center[i]] == (WHITE|KING)) nwkc++;
        }
    }
    eval+=(nbmc-nwmc)*mcv;
    eval+=(nbkc-nwkc)*kcv;
    
    /*edge*/
    for(i=0;i<14;i++)
    {
        if(b[edge[i]] != FREE)
        {
            if(b[edge[i]] == (BLACK|MAN)) nbme++;
            if(b[edge[i]] == (BLACK|KING)) nbke++;
            if(b[edge[i]] == (WHITE|MAN)) nwme++;
            if(b[edge[i]] == (WHITE|KING)) nwke++;
        }
    }
    eval-=(nbme-nwme)*mev;
    eval-=(nbke-nwke)*kev;
    
    
    
    /* tempo */
    for(i=5;i<41;i++)
    {
        if(b[i]== (BLACK | MAN))
            tempo+=row[i];
        if(b[i]== (WHITE | MAN))
            tempo-=7-row[i];
    }
    
    if(nm>=16) eval+=opening*tempo;
    if((nm<=15) && (nm>=12)) eval+=midgame*tempo;
    if(nm<9) eval+=endgame*tempo;
    
    
    
    for(i=0;i<4;i++)
    {
        if(nbk+nbm>nwk+nwm && nwk<3)
        {
            if(b[safeedge[i]]== (WHITE|KING))
                eval-=15;
        }
        if(nwk+nwm>nbk+nbm && nbk<3)
        {
            if(b[safeedge[i]]==(BLACK|KING))
                eval+=15;
        }
    }
    
    
    
    
    
    /* the move */
    if(nwm+nwk-nbk-nbm==0)
    {
        if(color==BLACK)
        {
            for(i=5;i<=8;i++)
            {
                for(j=0;j<4;j++)
                {
                    if(b[i+9*j]!=FREE) stonesinsystem++;
                }
            }
            if(stonesinsystem % 2)
            {
                if(nm+nk<=12) eval++;
                if(nm+nk<=10) eval++;
                if(nm+nk<=8) eval+=2;
                if(nm+nk<=6) eval+=2;
            }
            else
            {
                if(nm+nk<=12) eval--;
                if(nm+nk<=10) eval--;
                if(nm+nk<=8) eval-=2;
                if(nm+nk<=6) eval-=2;
            }
        }
        else
        {
            for(i=10;i<=13;i++)
            {
                for(j=0;j<4;j++)
                {
                    if(b[i+9*j]!=FREE) stonesinsystem++;
                }
            }
            if((stonesinsystem % 2) == 0)
            {
                if(nm+nk<=12) eval++;
                if(nm+nk<=10) eval++;
                if(nm+nk<=8) eval+=2;
                if(nm+nk<=6) eval+=2;
            }
            else
            {
                if(nm+nk<=12) eval--;
                if(nm+nk<=10) eval--;
                if(nm+nk<=8) eval-=2;
                if(nm+nk<=6) eval-=2;
            }
        }
    }
    
    
    return(eval);
}



/*-------------- PART III: MOVE GENERATION -----------------------------------*/

int generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color)
/*----------> purpose:generates all moves. no captures. returns number of moves
 ----------> version: 1.0
 ----------> date: 25th october 97 */
{
    int n=0,m;
    int i;
    
#ifdef STATISTICS
    generatemovelists++;
#endif
    
    if(color == BLACK)
    {
        for(i=5;i<=40;i++)
        {
            if( (b[i]&BLACK) !=0 )
            {
                if( (b[i]&MAN) !=0 )
                {
                    if( (b[i+4] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        if(i>=32) m=(BLACK|KING); else m=(BLACK|MAN); m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i+4;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(BLACK|MAN);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i+5] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        if(i>=32) m=(BLACK|KING); else m=(BLACK|MAN); m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i+5;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(BLACK|MAN);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                }
                if( (b[i]&KING) !=0 )
                {
                    if( (b[i+4] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(BLACK|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i+4;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(BLACK|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i+5] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(BLACK|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i+5;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(BLACK|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i-4] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(BLACK|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i-4;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(BLACK|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i-5] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(BLACK|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i-5;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(BLACK|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                }
            }
        }
    }
    else    /* color = WHITE */
    {
        for(i=5;i<=40;i++)
        {
            if( (b[i]&WHITE) !=0 )
            {
                if( (b[i]&MAN) !=0 )
                {
                    if( (b[i-4] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        if(i<=13) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i-4;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(WHITE|MAN);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i-5] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        if(i<=13) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i-5;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(WHITE|MAN);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                }
                if( (b[i]&KING) !=0 )  /* or else */
                {
                    if( (b[i+4] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(WHITE|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i+4;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(WHITE|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i+5] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(WHITE|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i+5;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(WHITE|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i-4] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(WHITE|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i-4;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(WHITE|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                    if( (b[i-5] & FREE) !=0 )
                    {
                        movelist[n].n=2;
                        m=(WHITE|KING);m=m<<8;
                        m+=FREE;m=m<<8;
                        m+=i-5;
                        movelist[n].m[1]=m;
                        m=FREE;m=m<<8;
                        m+=(WHITE|KING);m=m<<8;
                        m+=i;
                        movelist[n].m[0]=m;
                        n++;
                    }
                }
            }
        }
    }
    return(n);
}

int  generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color)
/*----------> purpose: generate all possible captures
 ----------> version: 1.0
 ----------> date: 25th october 97 */
{
    int n=0;
    int m;
    int i;
    int tmp;
    
#ifdef STATISTICS
    generatecapturelists++;
#endif
    
    if(color == BLACK)
    {
        for(i=5;i<=40;i++)
        {
            if( (b[i] & BLACK) !=0)
            {
                if( (b[i] & MAN) !=0)
                {
                    if( (b[i+4] & WHITE) !=0)
                    {
                        if( (b[i+8] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i+8;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(BLACK|MAN);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i+4];m=m<<8;
                            m+=i+4;
                            movelist[n].m[2]=m;
                            blackmancapture(b, &n, movelist, i+8);
                        }
                    }
                    if( (b[i+5] & WHITE) !=0)
                    {
                        if( (b[i+10] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i+10;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(BLACK|MAN);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i+5];m=m<<8;
                            m+=i+5;
                            movelist[n].m[2]=m;
                            blackmancapture(b, &n, movelist, i+10);
                        }
                    }
                }
                else /* b[i] is a KING */
                {
                    if( (b[i+4] & WHITE) !=0)
                    {
                        if( (b[i+8] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(BLACK|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i+8;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(BLACK|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i+4];m=m<<8;
                            m+=i+4;
                            movelist[n].m[2]=m;
                            tmp=b[i+4];
                            b[i+4]=FREE;
                            blackkingcapture(b, &n, movelist, i+8);
                            b[i+4]=tmp;
                        }
                    }
                    if( (b[i+5] & WHITE) !=0)
                    {
                        if( (b[i+10] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(BLACK|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i+10;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(BLACK|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i+5];m=m<<8;
                            m+=i+5;
                            movelist[n].m[2]=m;
                            tmp=b[i+5];
                            b[i+5]=FREE;
                            blackkingcapture(b, &n, movelist, i+10);
                            b[i+5]=tmp;
                        }
                    }
                    if( (b[i-4] & WHITE) !=0)
                    {
                        if( (b[i-8] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(BLACK|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i-8;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(BLACK|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i-4];m=m<<8;
                            m+=i-4;
                            movelist[n].m[2]=m;
                            tmp=b[i-4];
                            b[i-4]=FREE;
                            blackkingcapture(b, &n, movelist, i-8);
                            b[i-4]=tmp;
                        }
                    }
                    if( (b[i-5] & WHITE) !=0)
                    {
                        if( (b[i-10] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(BLACK|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i-10;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(BLACK|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i-5];m=m<<8;
                            m+=i-5;
                            movelist[n].m[2]=m;
                            tmp=b[i-5];
                            b[i-5]=FREE;
                            blackkingcapture(b, &n, movelist, i-10);
                            b[i-5]=tmp;
                        }
                    }
                }
            }
        }
    }
    else /* color is WHITE */
    {
        for(i=5;i<=40;i++)
        {
            if( (b[i] & WHITE) !=0)
            {
                if( (b[i] & MAN) !=0)
                {
                    if( (b[i-4] & BLACK) !=0)
                    {
                        if( (b[i-8] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i-8;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(WHITE|MAN);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i-4];m=m<<8;
                            m+=i-4;
                            movelist[n].m[2]=m;
                            whitemancapture(b, &n, movelist, i-8);
                        }
                    }
                    if( (b[i-5] & BLACK) !=0)
                    {
                        if( (b[i-10] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i-10;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(WHITE|MAN);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i-5];m=m<<8;
                            m+=i-5;
                            movelist[n].m[2]=m;
                            whitemancapture(b, &n, movelist, i-10);
                        }
                    }
                }
                else /* b[i] is a KING */
                {
                    if( (b[i+4] & BLACK) !=0)
                    {
                        if( (b[i+8] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(WHITE|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i+8;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(WHITE|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i+4];m=m<<8;
                            m+=i+4;
                            movelist[n].m[2]=m;
                            tmp=b[i+4];
                            b[i+4]=FREE;
                            whitekingcapture(b, &n, movelist, i+8);
                            b[i+4]=tmp;
                        }
                    }
                    if( (b[i+5] & BLACK) !=0)
                    {
                        if( (b[i+10] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(WHITE|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i+10;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(WHITE|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i+5];m=m<<8;
                            m+=i+5;
                            movelist[n].m[2]=m;
                            tmp=b[i+5];
                            b[i+5]=FREE;
                            whitekingcapture(b, &n, movelist, i+10);
                            b[i+5]=tmp;
                        }
                    }
                    if( (b[i-4] & BLACK) !=0)
                    {
                        if( (b[i-8] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(WHITE|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i-8;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(WHITE|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i-4];m=m<<8;
                            m+=i-4;
                            movelist[n].m[2]=m;
                            tmp=b[i-4];
                            b[i-4]=FREE;
                            whitekingcapture(b, &n, movelist, i-8);
                            b[i-4]=tmp;
                        }
                    }
                    if( (b[i-5] & BLACK) !=0)
                    {
                        if( (b[i-10] & FREE) !=0)
                        {
                            movelist[n].n=3;
                            m=(WHITE|KING);m=m<<8;
                            m+=FREE;m=m<<8;
                            m+=i-10;
                            movelist[n].m[1]=m;
                            m=FREE;m=m<<8;
                            m+=(WHITE|KING);m=m<<8;
                            m+=i;
                            movelist[n].m[0]=m;
                            m=FREE;m=m<<8;
                            m+=b[i-5];m=m<<8;
                            m+=i-5;
                            movelist[n].m[2]=m;
                            tmp=b[i-5];
                            b[i-5]=FREE;
                            whitekingcapture(b, &n, movelist, i-10);
                            b[i-5]=tmp;
                        }
                    }
                }
            }
        }
    }
    return(n);
}

void  blackmancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
{
    int m;
    int found=0;
    struct move2 move,orgmove;
    
    orgmove=movelist[*n];
    move=orgmove;
    
    if( (b[i+4] & WHITE) !=0)
    {
        if( (b[i+8] & FREE) !=0)
        {
            move.n++;
            if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
            m+=FREE;m=m<<8;
            m+=(i+8);
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i+4];m=m<<8;
            m+=(i+4);
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            blackmancapture(b, n, movelist, i+8);
        }
    }
    move=orgmove;
    if( (b[i+5] & WHITE) !=0)
    {
        if( (b[i+10] & FREE) !=0)
        {
            move.n++;
            if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
            m+=FREE;m=m<<8;
            m+=(i+10);
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i+5];m=m<<8;
            m+=(i+5);
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            blackmancapture(b, n, movelist, i+10);
        }
    }
    if(!found) (*n)++;
}

void  blackkingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
{
    int m;
    int tmp;
    int found=0;
    struct move2 move,orgmove;
    
    orgmove=movelist[*n];
    move=orgmove;
    
    if( (b[i-4] & WHITE) !=0)
    {
        if( (b[i-8] & FREE) !=0)
        {
            move.n++;
            m=(BLACK|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i-8;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i-4];m=m<<8;
            m+=i-4;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i-4];
            b[i-4]=FREE;
            blackkingcapture(b, n, movelist, i-8);
            b[i-4]=tmp;
        }
    }
    move=orgmove;
    if( (b[i-5] & WHITE) !=0)
    {
        if( (b[i-10] & FREE) !=0)
        {
            move.n++;
            m=(BLACK|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i-10;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i-5];m=m<<8;
            m+=i-5;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i-5];
            b[i-5]=FREE;
            blackkingcapture(b, n, movelist, i-10);
            b[i-5]=tmp;
        }
    }
    move = orgmove;
    if( (b[i+4] & WHITE) !=0)
    {
        if( (b[i+8] & FREE) !=0)
        {
            move.n++;
            m=(BLACK|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i+8;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i+4];m=m<<8;
            m+=i+4;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i+4];
            b[i+4]=FREE;
            blackkingcapture(b, n, movelist, i+8);
            b[i+4]=tmp;
        }
    }
    move=orgmove;
    if( (b[i+5] & WHITE) !=0)
    {
        if( (b[i+10] & FREE) !=0)
        {
            move.n++;
            m=(BLACK|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i+10;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i+5];m=m<<8;
            m+=i+5;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i+5];
            b[i+5]=FREE;
            blackkingcapture(b, n, movelist, i+10);
            b[i+5]=tmp;
        }
    }
    if(!found) (*n)++;
}

void  whitemancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
{
    int m;
    int found=0;
    struct move2 move,orgmove;
    
    orgmove=movelist[*n];
    move=orgmove;
    
    if( (b[i-4] & BLACK) !=0)
    {
        if( (b[i-8] & FREE) !=0)
        {
            move.n++;
            if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i-8;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i-4];m=m<<8;
            m+=i-4;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            whitemancapture(b, n, movelist, i-8);
        }
    }
    move=orgmove;
    if( (b[i-5] & BLACK) !=0)
    {
        if( (b[i-10] & FREE) !=0)
        {
            move.n++;
            if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i-10;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i-5];m=m<<8;
            m+=i-5;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            whitemancapture(b, n, movelist, i-10);
        }
    }
    if(!found) (*n)++;
}

void  whitekingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
{
    int m;
    int tmp;
    int found=0;
    struct move2 move,orgmove;
    
    orgmove=movelist[*n];
    move=orgmove;
    
    if( (b[i-4] & BLACK) !=0)
    {
        if( (b[i-8] & FREE) !=0)
        {
            move.n++;
            m=(WHITE|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i-8;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i-4];m=m<<8;
            m+=i-4;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i-4];
            b[i-4]=FREE;
            whitekingcapture(b, n, movelist, i-8);
            b[i-4]=tmp;
        }
    }
    move=orgmove;
    if( (b[i-5] & BLACK) !=0)
    {
        if( (b[i-10] & FREE) !=0)
        {
            move.n++;
            m=(WHITE|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i-10;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i-5];m=m<<8;
            m+=i-5;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i-5];
            b[i-5]=FREE;
            whitekingcapture(b, n, movelist, i-10);
            b[i-5]=tmp;
        }
    }
    move=orgmove;
    if( (b[i+4] & BLACK) !=0)
    {
        if( (b[i+8] & FREE) !=0)
        {
            move.n++;
            m=(WHITE|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i+8;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i+4];m=m<<8;
            m+=i+4;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i+4];
            b[i+4]=FREE;
            whitekingcapture(b, n, movelist, i+8);
            b[i+4]=tmp;
        }
    }
    move=orgmove;
    if( (b[i+5] & BLACK) !=0)
    {
        if( (b[i+10] & FREE) !=0)
        {
            move.n++;
            m=(WHITE|KING);m=m<<8;
            m+=FREE;m=m<<8;
            m+=i+10;
            move.m[1]=m;
            m=FREE;m=m<<8;
            m+=b[i+5];m=m<<8;
            m+=i+5;
            move.m[move.n-1]=m;
            found=1;
            movelist[*n]=move;
            tmp=b[i+5];
            b[i+5]=FREE;
            whitekingcapture(b, n, movelist, i+10);
            b[i+5]=tmp;
        }
    }
    if(!found) (*n)++;
}

int  testcapture(int b[46], int color)
/*----------> purpose: test if color has a capture on b
 ----------> version: 1.0
 ----------> date: 25th october 97 */
{
    int i;
    
#ifdef STATISTICS
    testcaptures++;
#endif
    if(color == BLACK)
    {
        for(i=5;i<=40;i++)
        {
            if( (b[i] & BLACK) !=0)
            {
                if( (b[i] & MAN) !=0)
                {
                    if( (b[i+4] & WHITE) !=0)
                    {
                        if( (b[i+8] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i+5] & WHITE) !=0)
                    {
                        if( (b[i+10] & FREE) !=0)
                            return(1);
                    }
                }
                else /* b[i] is a KING */
                {
                    if( (b[i+4] & WHITE) !=0)
                    {
                        if( (b[i+8] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i+5] & WHITE) !=0)
                    {
                        if( (b[i+10] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i-4] & WHITE) !=0)
                    {
                        if( (b[i-8] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i-5] & WHITE) !=0)
                    {
                        if( (b[i-10] & FREE) !=0)
                            return(1);
                    }
                }
            }
        }
    }
    else /* color is WHITE */
    {
        for(i=5;i<=40;i++)
        {
            if( (b[i] & WHITE) !=0)
            {
                if( (b[i] & MAN) !=0)
                {
                    if( (b[i-4] & BLACK) !=0)
                    {
                        if( (b[i-8] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i-5] & BLACK) !=0)
                    {
                        if( (b[i-10] & FREE) !=0)
                            return(1);
                    }
                }
                else /* b[i] is a KING */
                {
                    if( (b[i+4] & BLACK) !=0)
                    {
                        if( (b[i+8] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i+5] & BLACK) !=0)
                    {
                        if( (b[i+10] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i-4] & BLACK) !=0)
                    {
                        if( (b[i-8] & FREE) !=0)
                            return(1);
                    }
                    if( (b[i-5] & BLACK) !=0)
                    {
                        if( (b[i-10] & FREE) !=0)
                            return(1);
                    }
                }
            }
        }
    }
    return(0);
}
