// 
//
// checkers.h - standard include file for checkers projects, with all structures and constants. 
//
//

// number of pieces up to which we build databases.
#define MAXPIECES 8
// maximal number of a single piece - with this definition,
// if you build the 7-piece database, it will generate 4-3 but not 5-2. 
// for the 8-piece it will just generate the 4-4 database
// WARNING! the current code will not work for MAXPIECE > 4!
// NOT ANY MORE!
//#define MAXPIECE ((MAXPIECES+1)/2)
#define MAXPIECE 7


// ---------------------------------------------------------------------------------
// some compile switches to influence the behavior of builddb
//
// if this is set, builddb will not build databases, but give some statistics on the
// size of what it would be generating
//#define TESTSIZE

// use already present databases or start from scratch?
#define USEOLD

// best not to touch these :-)
#define REVERSE
#define SYMMETRIC
#define NOIO
#define UNMOVE




//---------------------------------------------------------------------------------
// constants - these are necessary for many things, and therefore also in this file
// do not change!

#define MAXMOVES 28 // maximum number of legal moves in any position

#define BLACK 0
#define WHITE 1
#define CC 1



// database scores 
#define UNKNOWN 0
#define WIN 1
#define LOSS 2
#define DRAW 3

//-------------------------------------------------------------------------------
// structures: define 32- and 64-bit integers, position and database info

typedef unsigned int int32;
typedef __int64 int64;

typedef struct pos
	{
	int32 bm;
	int32 bk;
	int32 wm;
	int32 wk;
	} position;

typedef struct sub
	{
	int bm;
	int bk;
	int wm;
	int wk;
	int maxbm;
	int maxwm;
	int64 databasesize;
	int *database;
	} subdb;


typedef position move;


//------------------------------------------------------------------------
// macros

#define pad16(x) (((x)/16+1)*16)
#define hiword(x) (((x)&0xFFFF0000)>>16)
#define loword(x) ((x)&0xFFFF)