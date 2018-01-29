//
//	dbmain.c
//	main file of builddb, the checkers endgame database builder
//	
//  (c) april 2002 Martin Fierz (checkers@fierz.ch)
//
//  this program is freeware / open source. if you have anything to contribute to it,
//  please let me know.
//
//  this program produces an english checkers endgame database in uncompressed format.
//  the algorithm is based on what is published by the chinook team in their paper on
//  the endgame database: http://www.cs.ualberta.ca/~jonathan/Papers/Papers/databases.ps
//  for details, look up everything in this paper. 
//
//  board is representation as in chinook db:
//  unsigned 32-bit integers are used to represent the board like this
//          WHITE
//     28  29  30  31           
//	 24  25  26  27           
//	   20  21  22  23          
//	 16  17  18  19           
//	   12  13  14  15          
//	  8   9  10  11          
//	    4   5   6   7           
//	  0   1   2   3           
//	      BLACK
//  a position is described by 4 integers, for black and white men and kings. 
//
//  my program does not incorporate
//  everything they did. it should build the 6-piece db in a few hours, the 7-piece db
//  in about 2 days and the 8-piece db in about 6 weeks on a XP1600+ with 640MB ram.
//
//  one important difference between this generator and the chinook generator is that
//  my range of indices is larger than the number of legal positions. this speeds up 
//  the position->index function, and gives slightly larger uncompressed databases. but since these
//  impossible positions can be set to any value during compression, the additional uncompressed storage
//  necessary will probably go away during compression.
//  the difference between my indexing scheme and the chinook scheme is that i do not
//  take into account interference between men of the two sides. therefore, it is possible
//  for an index to correspond to a position where there is both a black and a white man
//  on the same square. this can be detected easily by "if(p->bm & p->wm)". 
//  for more explanations on how to move from an index to a position and back, see the
//  file "index.c"
//
//  to adapt this program to other rules of checkers, all you need to do is change the
//  file "min_movegen.c" to reflect the rules of the version you want to compute a db
//  for. if you want to make use of the "undo move" function in the database generator,
//  you must produce a function called "makekingmovelist", which produces moves only
//  with kings. if you do not want to use this, just make that function return 0.
//  the db generator tacitly assumes that captures are forced. if this is not the
//  case in your version of checkers, you have to fix this.
// 
//  there are three variables to set (in checkers.h) which influence the database generation:
//  MAXPIECES is the maximum number of total pieces for which builddb will produce databases
//  MAXPIECE is the maximum number of a single piece type (e.g. black man) for which it will
//  produce databases. 
//  MAXPIECE is defined so that the builddb will not generate lopsided databases: when 
//  building the 8-piece database, it will only build the 4-4 db, but not 5-3 or even 
//  more lopsided databases. for databases with an odd number of pieces, like the 7-piece
//  db, it will build only 4-3, but not 5-2 etc.
//  when building the 8-piece db, it will build 4-2 and 4-1 databases, which are necessary
//  to generate the 4-4 db. 
//  UNMOVE can be set to use the unmove trick. recommended, because it reduces generation 
//  speed. on the down side, if you are adapting this code for other checkers versions, 
//  you will have to also adapt the "makekingmovelist" in min_movegen.c
//  
//  limitation warning: 
//  this code works for a maximum number of 4 checkers for each side. if you define MAXPIECE
//  to a value > 4, it will not work. for example, you cannot build
//  a 9-piece database just by setting MAXPIECES to 9, for the following two reasons:
//  -> if there are more than 4 checkers on one side, the expression to calculate the
//     index of the checkers won't work any more: this program tries to build a subdatabase
//     with all checkers on rank 0, which will not work if they don't fit!
//     this can easily be circumvented by changing the code for the index computation or by
//     just checking if there are 5 checkers for one side, and then not building that database
//  -> database sizes for the 8-piece database are within the range of an unsigned 32-bit integer.
//     for larger databases, 64-bit integers would have to be used. should be no problem as soon
//     as this program is compiled on a 64-bit OS
//  
//  suggested hardware:
//  running builddb with TESTSIZE defined will produce some output telling you how much
//  ram you should have for the build process to run efficiently. this is given by the constraint
//  that the propagation passes have to be run in memory. the lookup and noio passes can hit the
//  disk - since this only happens once for each db which is too large for memory, it doesn't matter.
//  but if e.g. 50 passes over that data hit the disk, it does matter...
//
//  the largest single database in the 4-4-piece db has 1.3 billion positions, which needs about
//  350MB ram. 
//  for a "release run" of the 9-piece database (computing only databases with 4-5 stones), the
//  largest database would have 13.5 (26.9) billion positions and would therefore need about 3.5GB (7) ram
//  for the 10-piece database, this would 140 billion positions, needing 35GB ram. 206BP (52GB).
//  the larger the databases get, the more my inefficient/lossy indexing scheme hurts.
//
//  builddb produces one file for every subdatabase it computes, which is named according to the 
//  number of men, kings, leading rank of men, and side to move
//  for example, db1201-30w.dat means 1 black man, 2 black kings, 0 white men, 1 white king, 
//  leading checker rank for black 3 (since there is only 1 man, this means it is on rank 3),
//  leading rank for white 0 (in fact, white has no man. in such cases, it uses 0), and white is
//  to move.
//  
//  the 8-piece database takes up about 37.5GB disk space. it can be compressed down to 4.3GB
//  with compressdb.
//
//
//  aloha
//    martin


//	addendum May 2005
//	I adapted builddb for suicide checkers and found the code to be horrible. I improved
//	its readability in many places, breaking down the huge function buildsubdbslice
//	into much smaller chunks. I also made builddb able to compute databases with more than
//	4 pieces for one side (MAXPIECES is no longer limited to 4).
//	I put the database name into a separate function, so changing the names of the output
//	files is only one small change in the function setdbname().
//	TODO: 
//		->	I should make all variables that could be larger than 32 bit of a type dbindex
//			and typedef that type to be int32 or int64, depending on how large the db is that
//			i'm building
//		->  ed gilbert reports that using "reverse passes" during propagation speeds up
//			the db build, should implement that 


#include <windows.h>
#include "checkers.h"
#include "dbmain.h"
#include "bool.h"
#include "index.h"
#include "min_movegen.h"
#include "lookup.h"

#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

double start,substart,progstart; // timers to measure the performance
int *currentdb_b, *currentdb_w;  // pointer to the databases which are currently being computed.
								 // used to prevent them from being unloaded if memory gets scarce
FILE *logfp;                     // file pointer to the build log file.  

#ifdef TESTSIZE
int64 maxdbsize = 0;             // some variables to keep track of the
int64 totaldbsize = 0;           // size of databases if TESTSIZE is defined
int64 symmetricsize = 0;         // 
#endif

int64 conversions=0;			// 2 counters to measure performance
int64 iterations=0;


// from lookup.c
// subdatabase records: hold info on a database slice, such as 2222.66
// every such record includes necessary information on this database, such
// as it's size, a pointer to it, if it is loaded etc. 
extern subdb subdatabase[MAXPIECE+1][MAXPIECE+1][MAXPIECE+1][MAXPIECE+1][7][7][2];

extern int bicoef[33][33];  // array of binomial coefficients. bicoef[n][k] is n!/(k!*(n-k)!).

int32 meminuse = 0;         // keep track of the total memory which is currently in use.
							// paging under windows is ok, but if you unload stuff before
							// it starts, you can do better than windows. for the 9-piece db,
							// this would have to be a int64.

main()
	{
	int n;

	printf("\nCheckers Endgame Database Builder by Martin Fierz 2002-2005");

	// clear perflog and winlog file, open buildlog file
	logfp = fopen("perflog.txt","w");
	fclose(logfp);
	logfp = fopen("winlog.txt","w");
	fclose(logfp);
	logfp = fopen("sizelog.txt","w");
	fclose(logfp);
	logfp = fopen("buildlog.txt","w");
	
	meminuse = 0;
	
	// initialize bool.c module
	initbool();

	// initialize lookup data
	initlookup();

	progstart = clock();

	// now the database building starts - build the 2, then 3, then 4, etc. -piece database
	for(n=2;n<=MAXPIECES;n++)
		{
		builddb(n);
		}

#ifdef TESTSIZE
	printf("\nsymmetric size is %I64i", symmetricsize);
	printf("\nnonsymmetric size is %I64i",totaldbsize);
	printf("\nmaximal db size is %I64i", maxdbsize);
	printf("\n\nbuilddb will need roughly %.1f MB RAM and %.3f GB harddisk space to run successfully",(float)maxdbsize/2.0/1024.0/1024.0,(float)totaldbsize/4.0/1024.0/1024.0/1024.0);
#endif

	fclose(logfp);
	exit(0);
	}

int builddb(int n)
	{
	// build all databases with n stones
	// do this by building all subdatabases with n stones organized into their bm bk wm wk category
	// first, it has to do the ones with the kings, and only later the ones with the stones.
	// this routine produces all kinds of databases, also those which we don't need to build.
	// they are discarded later

	int bm=0,bk=0,wm=0,wk=0;
	int div1, div2, div3;
	
#ifdef TESTSIZE
	totaldbsize = 0;
	symmetricsize = 0;
#endif

	fprintf(logfp,"\n\n******************************\n*** building %i-piece database\n******************************\n", n);
	for(div1=0;div1<=n;div1++)
		{
		for(div2=div1;div2<=n;div2++)
			{
			for(div3=div2;div3<=n;div3++)
				{
				bm=div1;
				bk=div2-div1;
				wm=div3-div2;
				wk=n-div3;
				buildsubdb(bm,bk,wm,wk);
				}
			}
		}
	return 1;
	}

int buildsubdb(int bm,int bk,int wm,int wk)
	{
	// buildsubdb builds a database with a fixed number of each kind of piece.
	// it further divides the size of the task by splitting up the databases
	// according to the leading rank of each checker (see chinook paper).

	int bmrank, wmrank; // hold the maximal rank of the white and black men
	FILE *fp;
	
	
	// is this a valid database?
	// one side has no pieces - we don't compute it:
	if((bm+bk==0) || (wm+wk==0)) 
		return 0;

	// white has more pieces than black - we don't compute it,
	// because we can reverse the board and lookup the value in that database
	if(bm+bk<wm+wk)
		return 0;

	// absurd databases: if maxpieces = 8, we don't do 5-3, 6-2, 7-1 or 6-1 or 5-1 etc.
	// this depends on definition of MAXPIECE in checkers.h. warning: don't make it larger
	// than 4, else the db generator will fail! TODO: enable more than 4 pieces per side!
	if(bm+bk>MAXPIECE)
		return 0;

	// more bk than wk in case of equal number of pieces - we don't compute it - again
	// we can reverse the position and look it up.
	if(wk+wm == bk+bm)
		{
		if(wk>bk)
			return 0;
		}

	fprintf(logfp,"\n\n*** building %i%i%i%i\n\n", bm, bk, wm, wk);

	start = clock();
	conversions = 0;
	iterations = 0;

	// now we are ready to build a subdatabaseslice: if there are checkers, we
	// have to do all the combinations of leading ranks. if there are only kings
	// for one side, it will use the database with leading rank "0" to name the
	// database.

	// the following 4 if's are mutually exclusive.
	// both sides only have kings
	if(bm == 0 && wm == 0)
		buildsubdbslice(bm,bk,wm,wk,0,0);
	
	// only black has checkers
	if(bm!=0 && wm==0)
		{
		// bmrank >= (bm-1)/4 should take care of 5 pieces?!
		for(bmrank=6;bmrank>=((bm-1)/4);bmrank--)
			buildsubdbslice(bm,bk,wm,wk,bmrank,0);
		}
	
	// only white has checkers
	if(wm!=0 && bm==0)
		{
		for(wmrank=6;wmrank>=((wm-1)/4);wmrank--)
			buildsubdbslice(bm,bk,wm,wk,0,wmrank);
		}

	// both sides have checkers
	if(bm!=0 && wm!=0)
		{
		for(bmrank=6;bmrank>=((bm-1)/4);bmrank--)
			{
			for(wmrank=6;wmrank>=((wm-1)/4);wmrank--)
				buildsubdbslice(bm,bk,wm,wk,bmrank,wmrank);
			}
		}

	fp = fopen("perflog.txt","a+");
	fprintf(fp,"\ntime %6.2lf (%6.2lf): bm %i bk %i wm %i wk %i (conv/s: %6i, iter/s: %6i) memused %i ", (clock()-progstart)/CLK_TCK,(clock()-start)/CLK_TCK,bm, bk, wm, wk, (int)(((double)conversions) / ((clock()-start)/CLK_TCK)),(int)(((double)iterations) / ((clock()-start)/CLK_TCK)), meminuse);
	fclose(fp);

	return 1;
	}

int buildsubdbslice(int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	//-----------------------------------------------------------------------------------------------------
	// buildsubdbslice is where the work is done. we want to calculate the 
	// database with bm black men, bk black kings, wm white men, and wk white kings.
	// in addition, for the men, we know that the MSB of p->bm is on bmrank, and
	// that the LSB of p->wm is on wmrank
	// this shrinks the size of the databases with men.
	//
	// buildsubdbslice will build two databases, one with white to move, one with black to move
	// if the position is symmetric, bm==wm, bk==wk, bmrank==wmrank, it only builds one with black to move
	//
	// buildsubdbslice should verify a finished database and return 1 if it is correct, 0 if not,
	// and buildsubdb should rebuild it if a 0 is returned. but i have not implemented this check!
	//-----------------------------------------------------------------------------------------------------
	
	int symmetric = 0;	// is the database symmetric?
	int64 dbsize;		// size of the database in number of positions
	int32 i;
	int present = 0;
	int32 memsize;		// will not work for databases which require more than 2-4GB ram.
	int done=0, pass=0;
	int32 wins=0, draws=0, losses=0;
	char dbname[64];
	FILE *fp;
	int loadblack = 0, loadwhite = 0;
		
	// timing
	substart = clock();

	//-----------------------------------------------------------------------------------------------------
	// check for a valid db - is it's reverse already computed?:
	//-----------------------------------------------------------------------------------------------------
	if(wm==bm && wk==bk)
		{
		if(wmrank>bmrank)
			return 0;
		}

	//-----------------------------------------------------------------------------------------------------
	// get the size of our database and do some housekeeping
	//-----------------------------------------------------------------------------------------------------

	dbsize = getdatabasesize(bm, bk, wm, wk, bmrank, wmrank);
	symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);
	//-----------------------------------------------------------------------------------------------------
	// if the database is symmetric, we don't have to do a lot of things.
	//-----------------------------------------------------------------------------------------------------

#ifdef TESTSIZE
	if(symmetric)
		symmetricsize+=dbsize;
	else
		{
		totaldbsize+=2*dbsize;
		dbsize*=2;
		}
	if(dbsize>maxdbsize)
		maxdbsize = dbsize;
	return 1;
#endif

	//-----------------------------------------------------------------------------------------------------
	// check if we already calculated this database if #USEOLD is defined
	// if yes, just return - if no, continue
	//-----------------------------------------------------------------------------------------------------
	
#ifdef USEOLD
	present = check_db_presence(bm, bk, wm, wk, bmrank, wmrank);
#endif
	if(present)
		{
		printf("\ndatabase already present: db%i%i%i%i-%i%i", bm,bk,wm,wk,bmrank,wmrank);
		return 1;
		}
	else
		printf("\ncalculating db%i%i%i%i-%i%i", bm,bk,wm,wk,bmrank,wmrank);


	//-----------------------------------------------------------------------------------------------------
	// start computing a new db slice. unload all databases currently in ram if we are 
	// above our memory limit:
	// there are two functions which free up memory: unloadall() unloads all databases
	// currently in memory, and memorypanic() unloads all except the 2 databases which
	// are currently being computed (only one, but black and white to move).
	//-----------------------------------------------------------------------------------------------------
	if(meminuse > MEMLIMIT)
		unloadall();

	//-----------------------------------------------------------------------------------------------------
	// allocate memory for the current dbslice and set the current database to this
	// database.
	//-----------------------------------------------------------------------------------------------------
	memsize = pad16(subdatabase[bm][bk][wm][wk][bmrank][wmrank][0].databasesize/4);
	meminuse += memsize;
	if(!symmetric)
		meminuse +=memsize;
	
	currentdb_b = VirtualAlloc(0, memsize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	subdatabase[bm][bk][wm][wk][bmrank][wmrank][0].database = currentdb_b;

	if(!symmetric)
		currentdb_w = VirtualAlloc(0, memsize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	else
		currentdb_w = NULL;

	subdatabase[bm][bk][wm][wk][bmrank][wmrank][1].database = currentdb_w;

	//-----------------------------------------------------------------------------------------------------
	// if there is more memory in use than MEMLIMIT (128MB?), deallocate all databases
	// except the current database with a call to memorypanic. databases will be loaded
	// again in the lookup process during the seed pass for this database. memorypanic
	// can also be called there, but only if malloc returns 0. so the basic memory strategy
	// is to examine memory usage before every new build, and this is normally the only
	// place where databases are deallocated. 
	//-----------------------------------------------------------------------------------------------------

	if(meminuse > MEMLIMIT)
		memorypanic();

	// if we cannot allocate memory now, then we are in trouble and exit the program.
	if(currentdb_b == NULL || (!symmetric && currentdb_w == NULL))
		{
		printf("\nerror on mem alloc");
		getch();
		exit(1);
		}

	
	//-----------------------------------------------------------------------------------------------------
	// generate some log output
	//-----------------------------------------------------------------------------------------------------
	printf("\n\nbuilding %i%i%i%i.%i%i", bm,bk, wm, wk, bmrank, wmrank);
	printf("\ndbsize: %i,  ",dbsize);
	printf("memory usage: %i KB", (meminuse)/1024);

	fprintf(logfp,"\n\n%.2lfs: building %i%i%i%i.%i%i", (clock()-start)/CLK_TCK, bm,bk, wm, wk, bmrank, wmrank);
	fprintf(logfp,"   dbsize: %i   memory usage: %iKB \n",dbsize, (meminuse/1024));
			
	
	//-----------------------------------------------------------------------------------------------------
	// STEP 1: initialize to 0 = UNKNOWN
	//-----------------------------------------------------------------------------------------------------
	set_db_to_UNKNOWN(currentdb_b, currentdb_w, bm, bk, wm, wk, bmrank, wmrank);


	//-----------------------------------------------------------------------------------------------------
	// 1b: set impossible positions to a WIN - i'm setting them to a known value, since the database
	// propagation pass checks for UNKNOWN or DRAWn positions to loop over - it can continue when it sees
	// a LOSS or a WIN
	//-----------------------------------------------------------------------------------------------------
	set_impossible_to(bm, bk, wm, wk, bmrank, wmrank, currentdb_b, currentdb_w, WIN);


	//-----------------------------------------------------------------------------------------------------
	// STEP 2: "seed pass": resolve captures 
	//-----------------------------------------------------------------------------------------------------
	resolvecaptures(bm, bk, wm, wk, bmrank, wmrank, currentdb_b, currentdb_w);


	//-----------------------------------------------------------------------------------------------------
	//  STEP 3:		do an IO pass as in chinook paper: only look at moves which convert
	//				into databases we have already calculated, i.e. moves which increas
	//				the leading checker rank or promote. if such a move leads to a WIN,
	//				save it as a win, if it leads to a DRAW, we save it as a DRAW, but
	//				keep in mind that a DRAW does not mean the position is resolved, 
	//				just that it is AT LEAST a DRAW
	//				if there are no men for the side to move, then there is no point in doing 
	//				this - you cannot convert to another database
	//-----------------------------------------------------------------------------------------------------
	if(bm)
		IOpassblack(currentdb_b, bm, bk, wm, wk, bmrank, wmrank);
			
	if(!symmetric && wm)
		IOpasswhite(currentdb_w, bm, bk, wm, wk, bmrank, wmrank);
	
	// all smaller db's will no longer be used, again, memorypanic
	if(meminuse > MEMLIMIT)
		memorypanic();
		
	//-----------------------------------------------------------------------------------------------------
	// STEP 4:	do a series of propagation passes until no new values are found
	//			here, only WINS and LOSSES can be resolved. DRAWS are meaningless
	//          in the "noio" framework and can not be propagated
	//			!! this loop takes most of the time of builddb. optimize it !!
	//-----------------------------------------------------------------------------------------------------
	
	printf("\n\npropagating %i%i%i%i.%i%i", bm,bk, wm, wk, bmrank, wmrank);
	substart = clock();
	conversions = 0;
	iterations = 0;

	while(!done)
		{
		//done = 1;
		pass++;
		// 4a: propagation for BLACK
	
		printf("\nblack pass %i:",pass);
		done = propagateblack(currentdb_b, currentdb_w, bm, bk, wm, wk, bmrank, wmrank);

		//-----------------------------------------------------------------------------------------------------
		// 4b: propagation pass for WHITE
		//-----------------------------------------------------------------------------------------------------
		if(!symmetric)
			{
			printf("\nwhite pass %i:",pass);
			done &= propagatewhite(currentdb_b, currentdb_w, bm, bk, wm, wk, bmrank, wmrank);
			}
		}

	//-----------------------------------------------------------------------------------------------------
	// STEP 5: set all unknown values to draw
	//-----------------------------------------------------------------------------------------------------
	set_unknown_to_draw(currentdb_b, dbsize);
	if(!symmetric)
		set_unknown_to_draw(currentdb_w, dbsize);
	

	//-----------------------------------------------------------------------------------------------------
	// STEP 6: set all impossible positions to UNKNOWN
	//-----------------------------------------------------------------------------------------------------
	set_impossible_to(bm, bk, wm, wk, bmrank, wmrank, currentdb_b, currentdb_w, UNKNOWN);


	//-----------------------------------------------------------------------------------------------------
	// STEP 7: save file to disk
	//-----------------------------------------------------------------------------------------------------
	
	// TODO: stuff into savetodisk()

	setdbname(bm, bk, wm, wk, bmrank, wmrank, BLACK, dbname);
	fp = fopen(dbname,"wb");
	fwrite(currentdb_b, 1, memsize, fp);
	fclose(fp);

	if(!symmetric)
		{
		setdbname(bm, bk, wm, wk, bmrank, wmrank, WHITE, dbname);
		fp = fopen(dbname,"wb");
		fwrite(currentdb_w, 1, memsize, fp);
		fclose(fp);
		}

	//-----------------------------------------------------------------------------------------------------
	// STEP 8: check database integrity - todo!
	//-----------------------------------------------------------------------------------------------------
	
	// if not ok, return 0. this also necessitates that the calling routine, buildsubdb, recognizes the
	// return value and repeats the calculation until it is correct
	// 
	// i computed the 8-piece db for english checkers without a check. i verified every position in the 
	// 2-6-piece db against the chinook db. i compared win/loss/draw counts for every database slice
	// against the nemesis database - with matching counts. to the best of my knowledge, my 8-piece db
	// is the first to be correct on the first try (not so: chinook, WCC, nemesis). even though they all
	// had verification in...

	//-----------------------------------------------------------------------------------------------------
	// STEP 9: write stats to disk to check win, loss and draw counts between versions
	//-----------------------------------------------------------------------------------------------------
	// TODO: stuff into writestats()

	wins=0;draws=0;losses=0;
	for(i=0;i<dbsize;i++)
		{
		switch(getdatabasevalue(currentdb_b,i))
			{
			case WIN: 
				wins++;
				break;
			case DRAW:
				draws++;
				break;
			case LOSS:
				losses++;
			}
		}
	fp = fopen("winlog.txt","a");
	fprintf(fp,"\n%i%i%i%i-%i%ib: %i wins, %i losses, %i draws", bm, bk, wm, wk, bmrank, wmrank, wins, losses, draws);
	fclose(fp);

	
	if(!symmetric)
		{
		wins=0; 
		losses=0;
		draws = 0;
		for(i=0;i<dbsize;i++)
			{
			switch(getdatabasevalue(currentdb_w,i))
				{
				case WIN: 
					wins++;
					break;
				case DRAW:
					draws++;
					break;
				case LOSS:
					losses++;
				}
			}	
		fp = fopen("winlog.txt","a");
		fprintf(fp,"\n%i%i%i%i-%i%iw: %i wins, %i losses, %i draws", bm, bk, wm, wk, bmrank, wmrank, wins, losses, draws);
		fclose(fp);
		}
	return 1;
	}

int issymmetric(int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	if(bm==wm && bk==wk && bmrank==wmrank)
		return 1;
	else
		return 0;
	}


set_db_to_UNKNOWN(int *currentdb_b, int *currentdb_w, int bm, int bk, int wm, int wk, 
					int bmrank, int wmrank)
	{
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);

	memset(currentdb_b, 0, pad16(subdatabase[bm][bk][wm][wk][bmrank][wmrank][0].databasesize/4));
	if(!symmetric)
		memset(currentdb_w, 0, pad16(subdatabase[bm][bk][wm][wk][bmrank][wmrank][1].databasesize/4));
	return 1;
	}

int set_impossible_to(int bm, int bk, int wm, int wk, int bmrank, int wmrank, 
						  int *currentdb_b, int *currentdb_w, int result)
	{
	int index;
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);
	int64 dbsize = getdatabasesize(bm, bk, wm, wk, bmrank, wmrank);
	position p; 

	
	if(bmrank+wmrank>6)
		{
		printf("\nsetting impossible positions to a known result");
		for(index=0; index<dbsize; index++)
			{
			indextoposition(index, &p, bm, bk, wm, wk, bmrank, wmrank);
			if(p.bm & p.wm)
				{
				setdatabasevalue(currentdb_b, index, result);
				if (!symmetric)
					setdatabasevalue(currentdb_w, index, result);
				}
			}
		}
	return 1;
	}


int resolvecaptures(int bm, int bk, int wm, int wk, int bmrank, int wmrank, 
					int *currentdb_b, int *currentdb_w)
	{
	int j, n;
	int32 index;
	int64 dbsize = getdatabasesize(bm, bk, wm, wk, bmrank, wmrank);
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);
	int bestvalue;
	int value;
	position p;
	move movelist[MAXMOVES];

	// 2a: resolve captures on BLACK database
	printf("\nresolving captures on black database");

	for(index=0;index<dbsize;index++)
		{
		indextoposition(index, &p, bm, bk, wm, wk, bmrank, wmrank);
		conversions++;
		iterations++;
		// check for impossible positions
		if(p.bm & p.wm)
			continue;
		// make a movelist
		n=makecapturelist(&p, movelist, BLACK);
		if(n)
			{
			bestvalue = LOSS;
			for(j=0;j<n;j++)
				{
				togglemove(&p, &movelist[j]);
				value = lookup(&p, WHITE);
				togglemove(&p, &movelist[j]);

				if(value == LOSS)
					{
					bestvalue = WIN;
					break;
					}
				if(value == DRAW)
					bestvalue=DRAW;
				}
			setdatabasevalue(currentdb_b, index, bestvalue);
			}
		}

	// 2b resolve captures on WHITE database
	// we don't need the other subdbs any more, memorypanic
	if(meminuse > MEMLIMIT)
		memorypanic();
	

	if(!symmetric)
		{
		printf("\nresolving captures on white database");
		for(index=0;index<dbsize;index++)
			{
			indextoposition(index, &p, bm, bk, wm, wk, bmrank, wmrank);
			
			iterations++;
			conversions++;

			// collision detection
			if(p.bm & p.wm)
				continue;
			// make a movelist
			n=makecapturelist(&p, movelist, WHITE);
			if(n)
				{
				bestvalue = LOSS;
				for(j=0;j<n;j++)
					{
					togglemove(&p, &movelist[j]);
					value = lookup(&p, BLACK);
					togglemove(&p, &movelist[j]);

					if(value == LOSS)
						{
						bestvalue = WIN;
						break;
						}
					if(value == DRAW)
						bestvalue=DRAW;
					}
				setdatabasevalue(currentdb_w, index, bestvalue);
				}
			}
		}

	fprintf(logfp,"\n%.2lfs: seed pass complete", (clock()-start)/CLK_TCK);	
	
	printf("\n\nseed pass black:");
	getstats(currentdb_b, dbsize);
	if(!symmetric)
		{
		printf("\nseed pass white:");
		getstats(currentdb_w, dbsize);
		}

	printf("\n           time %.2lf, conv/s: %i,  (bm %i bk %i wm %i wk %i)", (clock()-start)/CLK_TCK, (int)(((double)conversions) / ((clock()-start)/CLK_TCK)), bm, bk, wm, wk);

	// after the seed pass, we will no longer need the smaller databases,
	// so a call to memorypanic makes sense 
	if(meminuse > MEMLIMIT)
		memorypanic();
	return 1;
	}



int IOpassblack(int *currentdb, int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	int64 dbsize = getdatabasesize(bm, bk, wm, wk, bmrank, wmrank);
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);
	int32 i,j;
	int value, bestvalue;
	position p;
	move movelist[MAXMOVES];
	int n, conv;
	int conversion;

	substart = clock();
	
	
	// 3a: BLACK (IO only happens if there are men on the board!)
	printf("\n\nIO pass on %i%i%i%i.%i%i", bm,bk, wm, wk, bmrank, wmrank);
	printf("\ndoing black IO pass");
	
	for(i=0; i<dbsize; i++)
		{
		iterations++;

		//-----------------------------------------------------------------------------------------------------
		// for all positions do a lookup if the value is not known
		// here, DRAW is a proper value, coming from trades in other databases
		//-----------------------------------------------------------------------------------------------------	
		if(getdatabasevalue(currentdb, i) == UNKNOWN)
			{
			conversions++;
			
			indextoposition(i, &p, bm, bk, wm, wk, bmrank, wmrank);
			
			bestvalue = LOSS;
		
			n = makemovelist(&p, movelist, BLACK); // BLACK
			conv = 0;
			// look up all successors and propagate their values
			for(j=0; j<n; j++)
				{
				// if ! conversion continue
				conversion = 0;
				if(movelist[j].bm)
					{
					if( MSB(movelist[j].bm)/4 > bmrank)
						conversion = 1;
					if( movelist[j].bk)
						conversion = 1;
					}
				
				if(!conversion)
					continue;

				conv++;
				togglemove(&p, &movelist[j]);
				//-----------------------------------------------------------------------------------------------------
				// after domove, look up the position with the other side to move
				// but only if it was really a conversion move, which means that either
				// we have a new king for black, or we moved the man which was most advanced
				// to a new row.
				// bmrank = MSB(p.bm) / 4
				//-----------------------------------------------------------------------------------------------------
			
				// arriving here means that either we have a new king or bm/wmrank has changed
				value = lookup(&p, WHITE);
				togglemove(&p, &movelist[j]);
				if(value == LOSS)
					{
					bestvalue = WIN;
					break;
					}

				if(value == DRAW)
					bestvalue = DRAW;
				}

			// change the next statement to = WIN in case of suicide checkers
			if(n == 0)
				bestvalue = LOSS;

			// set value in case it's a win or if all successors are known:
			if(conv == n && bestvalue == LOSS)
				{
				setdatabasevalue(currentdb, i, LOSS);
				//losses++;
				}
			else if(bestvalue!=LOSS)
				{
				setdatabasevalue(currentdb, i, bestvalue);
				//if(bestvalue == WIN)
				//	wins++;
				}
			}
		}

	//-----------------------------------------------------------------------------------------------------
	// now, all subdatabases which we loaded for the black pass are useless
	// so again, a call to memorypanic makes sense:
	//-----------------------------------------------------------------------------------------------------
	if(meminuse > MEMLIMIT)
		memorypanic();


	printf("\nIO Pass done:");
	getstats(currentdb, dbsize);
	printf("\n         time %.2lf, conv/s: %i, iter/s: %i (bm %i bk %i wm %i wk %i)", (clock()-substart)/CLK_TCK, (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-start)/CLK_TCK)), bm, bk, wm, wk);

//		fprintf(logfp,"\n%.2lfs: black IO pass complete   %i wins %i draws %i losses of %i", (clock()-substart)/CLK_TCK, wins, draws, losses, dbsize);
	fprintf(logfp,"conv/s: %i, iter/s: %i", (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-substart)/CLK_TCK)), bm, bk, wm, wk);

	return 1;
	}

int IOpasswhite(int *currentdb, int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	int64 dbsize = getdatabasesize(bm, bk, wm, wk, bmrank, wmrank);
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);
	int32 i,j;
	int value, bestvalue;
	position p;
	move movelist[MAXMOVES];
	int n, conv;
	int conversion;

	substart = clock();

	for(i=0;i<dbsize;i++)
		{
		iterations++;

		//-----------------------------------------------------------------------------------------------------
		// for all positions do a lookup
		// only do something if the position is not known
		//-----------------------------------------------------------------------------------------------------
		if(getdatabasevalue(currentdb, i) == UNKNOWN)
			{
			conversions++;
			conv = 0;
			bestvalue = LOSS;
			
			indextoposition(i, &p, bm, bk, wm, wk, bmrank, wmrank);
	
			n=makemovelist(&p, movelist, WHITE);
			// look up all successors and propagate their values
			for(j=0;j<n;j++)
				{
				// if ! conversion continue
				conversion = 0;

				if(movelist[j].wm)
					{
					//-----------------------------------------------------------------------------------------------------
					// tricky!! if movelist[j].wm was 0, then this expression
					// for the rank computation is wrong, as LSB returns -1, giving 
					// 32/4 = 8 > wmrank always!
					//-----------------------------------------------------------------------------------------------------
					if( (31-LSB(movelist[j].wm))/4 > wmrank)
						conversion = 1;
					if( movelist[j].wk) // && movelist[j].wm, but that is obviously aalready the case
						conversion = 1;
					}

				if(!conversion)
					continue;
				
				conv++;
				
				// after domove, look up the position with the other side to move
				togglemove(&p, &movelist[j]);
				value = lookup(&p, WHITE^CC);
				togglemove(&p, &movelist[j]);

				if(value == LOSS)
					{
					bestvalue = WIN;
					break;
					}
				if(value == DRAW)
					bestvalue = DRAW;
				}
			// for suicide checkers change next line to bestvalue = WIN
			if(n == 0)
				bestvalue = LOSS;

			// set value in case it's a win or if all successors are known:
			if(conv == n && bestvalue == LOSS)
				{
				setdatabasevalue(currentdb, i, LOSS);
				//losses++;
				}
				
			else if(bestvalue != LOSS)
				{
				setdatabasevalue(currentdb, i, bestvalue);
				//if(bestvalue==WIN)
				//	wins++;
				}
			}
		}
//	printf("\nwhite IO pass: %i wins, % i draws, %i losses of %i",wins, draws, losses, dbsize);
	printf("\n         time %.2lf, conv/s: %i, iter/s: %i (bm %i bk %i wm %i wk %i)", (clock()-substart)/CLK_TCK, (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-start)/CLK_TCK)), bm, bk, wm, wk);

	//fprintf(logfp,"\n%.2lfs: white IO pass complete   %i wins %i draws %i losses of %i", (clock()-substart)/CLK_TCK, wins, draws, losses, dbsize);
	fprintf(logfp,"conv/s: %i, iter/s: %i", (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-substart)/CLK_TCK)), bm, bk, wm, wk);
	
	return 1;	
	} 
	

int propagateblack(int *currentdb_b, int *currentdb_w, int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	int i,j,n;
	int oldvalue;
	int bestvalue;
	int value;
	//int conversion;
	int64 dbsize = getdatabasesize(bm, bk, wm, wk, bmrank, wmrank);
	int32 index;
	position p,q;
	move movelist[MAXMOVES];
	int done = 1;
	int wins = 0;
	int draws = 0;
	int losses = 0;
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);

	for(i=0;i<dbsize;i++)
		{
		iterations++;

		//-----------------------------------------------------------------------------------------------------
		// for all positions do a lookup
		// only do something if the value of the position is not known, or draw
		// draw indicates that it has a successor in another subslice than it's mirror
		// which is a draw, thus guaranteeing that this position's value is at least a
		// draw
		//-----------------------------------------------------------------------------------------------------

		oldvalue = getdatabasevalue(currentdb_b, i);
		
		if(oldvalue == UNKNOWN || oldvalue == DRAW)  // if i define WIN 0 LOSS 1 DRAW 2 UNKNOWN 3 i can do (if oldvalue &2)
			{
			conversions++;
			// indextoposition does a lot of unnecessary work: it computes index ranges which are identical
			// for the entire db. change? if i change this, it will be a lot more difficult to read, and
			// to fit together with the db compressor :-(
			// besides, the position-to-index thing costs a lot of time too
			indextoposition(i, &p, bm, bk, wm, wk, bmrank, wmrank);

			//-----------------------------------------------------------------------------------------------------
			// if it's a capture: continue - captures which are DRAWS have already been resolved
			//-----------------------------------------------------------------------------------------------------
			if (oldvalue == DRAW) 
				{
				if(testcapture(&p, BLACK))
					continue;
				}

			bestvalue = LOSS;
			
			n = makemovelist(&p, movelist, BLACK);
			
			// look up all successors and propagate their values
			for(j=0;j<n;j++)
				{
				// if this move takes us into another database, don't look up because we did this 
				// in IOpass() if conversion continue
				//conversion = 0;
				if(movelist[j].bm)
					{
					// TODO i could remove this MSB by getting myself a bitboard
					// with ranks > bmrank and test with a simple &:
					// if((movelist[j].bm & conversionranks) | movelist[j].bk)
					if( MSB(movelist[j].bm)/4 > bmrank || movelist[j].bk)
						continue;
					}

				// domove, look up the position with the other side to move
				togglemove(&p, &movelist[j]);
				value = lookup(&p, WHITE);
				togglemove(&p, &movelist[j]);

				// if we find a successor which is a loss, we have a win and can stop
				if(value == LOSS)
					{
					bestvalue = WIN;
					wins++;
					break;
					}

				// else, we just try to improve the best value so far
				// if any of the successor values is UNKNOWN, bestvalue is set to UNKNOWN.
				// it remains there once it is set to UNKNOWN, unless a LOSS comes along.
				if(value == UNKNOWN)
					bestvalue = UNKNOWN;

				else if(bestvalue == LOSS && value==DRAW)
					bestvalue = DRAW;
				}

			if(oldvalue == DRAW)
				// there is a conversion move leading to a draw - can we improve?
				{
				if(bestvalue == WIN)
					{
					setdatabasevalue(currentdb_b, i, bestvalue);
					done = 0;
					wins++;
					}
				}
			else // this means: oldvalue UNKNOWN
				{
				//-----------------------------------------------------------------------------------------------------
				// there is either no conversion move, or only such which lead to a loss.
				// problem with DRAW: if one successor is marked as a draw, but is a loss 
				// (because it is a position where a conversion will lead to a draw), and
				// all other positions are LOSSES, then this will give bestvalue = draw. 
				// when in fact we don't know it.
				//-----------------------------------------------------------------------------------------------------
				
				// if any successor value was unknown, so will bestvalue be - except if there was a win.
				if(bestvalue == WIN)
					{
					setdatabasevalue(currentdb_b, i, bestvalue);
					wins++;
					done = 0;
					}
				else if(bestvalue == LOSS)
					{
					setdatabasevalue(currentdb_b, i, bestvalue);
					losses++;
					done=0;

#ifdef UNMOVE
					//-----------------------------------------------------------------------------------------------------
					// we retrace our steps into parent positions which may be wins
					// BLACK to move is lost in this position, i.e. any white move 
					// which leads to this position is a win. we don't want to move
					// out of the database we are computing, so we only do white king
					// moves.
					//-----------------------------------------------------------------------------------------------------
					n=makekingmovelist(&p, movelist, WHITE);
					for(j=0;j<n;j++)
						{
						togglemove(&p, &movelist[j]);
						if(!symmetric)
							{
							// todo: use adjacency trick
							if(!testcapture(&p, WHITE))
								{
								positiontoindex(&p, &index);
								setdatabasevalue(currentdb_w, index, WIN);
								}
							}
						else
							{
							q.bm=revert(p.wm);
							q.bk=revert(p.wk);
							q.wm=revert(p.bm);
							q.wk=revert(p.bk);
							if(!testcapture(&q,BLACK))
								{
								positiontoindex(&q, &index);
								setdatabasevalue(currentdb_b, index, WIN);
								}
							}
						togglemove(&p, &movelist[j]);
						}
#endif // UNMOVE
					}
				}
			}
		}

	printf(" %i new wins, % i new draws, %i new losses of %i", wins, draws, losses, dbsize);
	printf("\n         time %.2lf, conv/s: %i, iter/s: %i (bm %i bk %i wm %i wk %i)", (clock()-substart)/CLK_TCK, (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-start)/CLK_TCK)), bm, bk, wm, wk);

	fprintf(logfp,"\n%.2lfs:   %i wins %i draws %i losses of %i", (clock()-substart)/CLK_TCK, wins, draws, losses, dbsize);
	fprintf(logfp,"conv/s: %i, iter/s: %i", (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-substart)/CLK_TCK)), bm, bk, wm, wk);

	return done;
	}


int propagatewhite(int *currentdb_b, int *currentdb_w, int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	int i,j,n;
	int oldvalue;
	int bestvalue;
	int value;
//	int conversion;
	int64 dbsize = getdatabasesize(bm, bk, wm, wk, bmrank, wmrank);
	int32 index;
	position p;
	move movelist[MAXMOVES];
	int done = 1;
	int wins = 0;
	int draws = 0;
	int losses = 0;
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);


	
	for(i=0;i<dbsize;i++)
		{
		iterations++;

		//-----------------------------------------------------------------------------------------------------
		// for all positions do a lookup
		// only do something if the value of the position is not known, or draw
		// draw indicates that it has a successor in another subslice than its mirror
		// which is a draw, thus guaranteeing that this position's value is at least a
		// draw
		//-----------------------------------------------------------------------------------------------------

		oldvalue = getdatabasevalue(currentdb_w, i);
		if(oldvalue == UNKNOWN || oldvalue == DRAW)
			{
			conversions++;
			// again, this indextoposition does too much work!
			indextoposition(i, &p, bm, bk, wm, wk, bmrank, wmrank);

			// if it's a capture: continue
			if (oldvalue == DRAW) 
				{
				if(testcapture(&p, WHITE))
					continue;
				}

			bestvalue = LOSS;

			n = makemovelist(&p, movelist, WHITE);

			// look up all successors and propagate their values
			for(j=0; j<n; j++)
				{
				// now check if this move takes us into another database, if yes,
				// don't look up because we did this before
				// statement below: if (conversion) continue

				// if conversion continue
				//conversion = 0;
				if(movelist[j].wm)
					{
					// TODO: create a conversion bitmap to test with an &
					if( (31-LSB(movelist[j].wm))/4 > wmrank || movelist[j].wk)
						continue;
					}

				togglemove(&p, &movelist[j]);
				// after domove, look up the position with the other side to move
				value = lookup(&p, BLACK);
				togglemove(&p, &movelist[j]);

				// if we find a successor which is a loss, we have a win and can stop
				if(value == LOSS)
					{
					bestvalue = WIN;
					break;
					}

				// else, we just try to improve the best value so far
				// if any of the successor values is UNKNOWN, bestvalue is set to UNKNOWN.
				if(value == UNKNOWN)
					bestvalue = UNKNOWN;

				if(bestvalue == LOSS && value==DRAW)
					bestvalue = DRAW;
				}

			// now, what can we do?
			if(oldvalue == DRAW)
				// there is a conversion move leading to a draw - can we improve?
				{
				if(bestvalue == WIN)
					{
					setdatabasevalue(currentdb_w, i, bestvalue);
					wins++;
					// TODO: done = 0 could be set at the end of the function if the number of
					// wins is not the same as the old number of wins
					done = 0;
					}
				}
			else // oldvalue UNKNONW
				{
				// there is either no conversion move, or only such which lead to a loss.
				// we cannot set draws. 
				// if any successor value was unknown, so will bestvalue be - except if there was a win.
				if(bestvalue == WIN)
					{
					setdatabasevalue(currentdb_w, i, bestvalue);
					done = 0;
					wins++;
					}
				if(bestvalue == LOSS)
					{
					setdatabasevalue(currentdb_w, i, bestvalue);
					done = 0;
					losses++;
#ifdef UNMOVE
				// we retrace our steps into parent positions which may be wins
				// WHITE to move is lost in this position, i.e. any black move 
				// which leads to this position is a win
				
					n = makekingmovelist(&p, movelist, BLACK);
					
					for(j=0;j<n;j++)
						{
						togglemove(&p, &movelist[j]);
						// this is a position with black to move, in which
						// he can move into the win - but only if he has no capture!
						
						if(!testcapture(&p,BLACK))
							{
							positiontoindex(&p, &index);
							setdatabasevalue(currentdb_b, index, WIN);
							}
						togglemove(&p, &movelist[j]);
						}
#endif  // UNMOVE
					}
				}
			}
		}

	printf("%i new wins, % i new draws, %i new losses of %i",wins, draws, losses, dbsize);
	printf("\n         time %.2lf, conv/s: %i, iter/s: %i (bm %i bk %i wm %i wk %i)", (clock()-substart)/CLK_TCK, (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-start)/CLK_TCK)), bm, bk, wm, wk);

	fprintf(logfp,"\n%.2lfs: pass complete  %i wins %i draws %i losses of %i", (clock()-substart)/CLK_TCK, wins, draws, losses, dbsize);
	fprintf(logfp,"conv/s: %i, iter/s: %i", (int)(((double)conversions) / ((clock()-substart)/CLK_TCK)),(int)(((double)iterations) / ((clock()-substart)/CLK_TCK)), bm, bk, wm, wk);
	
	return done;
	}

int set_unknown_to_draw(int *currentdb, int64 dbsize)
	{
	int32 i;

	for(i=0;i<dbsize;i++)
		{
		if(getdatabasevalue(currentdb, i)==UNKNOWN)
			setdatabasevalue(currentdb, i, DRAW);
		}
	return 1;
	}

int setdbname(int bm, int bk, int wm, int wk, int bmrank, int wmrank, int color, char *dbname)
	{
	// SUICIDE CHECKERS: use "scdb%i..."
	// regular checkers: use "db%i..."
	if(color == BLACK)
		sprintf(dbname,"raw\\db%i%i%i%i-%i%ib.dat", bm,bk,wm,wk,bmrank,wmrank);
	else
		sprintf(dbname,"raw\\db%i%i%i%i-%i%iw.dat", bm,bk,wm,wk,bmrank,wmrank);
	return 1;
	}

	
int getstats(int *currentdb, int64 dbsize)
	{
	int i;
	int wins=0, draws=0, losses=0;

	for(i=0;i<dbsize;i++)
		{
		switch(getdatabasevalue(currentdb,i))
			{
			case WIN: 
				wins++;
				break;
			case DRAW:
				draws++;
				break;
			case LOSS:
				losses++;
			}
		}
	printf("wins %i draws %i losses %i of %I64i", wins, draws, losses, dbsize);

	return 1;
	}



int setdatabasevalue(int *database, int index, int value)

	//-----------------------------------------------------------------------------------------------------
	// sets a value in the database
	//-----------------------------------------------------------------------------------------------------

	{
	int32 tmp;
	int32 mask;

	tmp = database[index/16];
	mask = 3 << (2*(index%16));
	mask = ~mask;
	tmp &= mask;

	tmp+= value << (2*(index%16));
	database[index/16] = tmp;
	
	return 1;
	}

int check_db_presence(int bm, int bk, int wm, int wk, int bmrank, int wmrank)
	{
	char dbname[256];
	FILE *fp;
	int loadblack = 0;
	int loadwhite = 0;
	int symmetric = issymmetric(bm, bk, wm, wk, bmrank, wmrank);

	setdbname(bm, bk, wm, wk, bmrank, wmrank, BLACK, dbname);
	fp = fopen(dbname,"rb");
	if(fp!=NULL)
		{
		loadblack = 1;
		fclose(fp);
		}
	
	setdbname(bm, bk, wm, wk, bmrank, wmrank, WHITE, dbname);
	
	fp = fopen(dbname,"rb");
	if(fp!=NULL)
		{
		loadwhite = 1;
		fclose(fp);
		}	

	if(symmetric)
		loadwhite = 1;

	if(loadblack && loadwhite) 
		// if both white and black database are present
		// (or if only black db is present in symmetric case), we return and
		// don't compute anything. 
		// if only one of the two is present, we compute both again.
		{
		return 1;
		}
	return 0;
	}



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
									VirtualFree(subdatabase[i][j][k][l][m][n][0].database , 0, MEM_RELEASE);
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
									VirtualFree(subdatabase[i][j][k][l][m][n][1].database , 0, MEM_RELEASE);
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


int unloadall(void)
	{
	//-----------------------------------------------------------------------------------------------------
	// unloads all databases from memory
	// this can be called to free all memory
	//-----------------------------------------------------------------------------------------------------
	int i,j,k,l,m,n;	
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
								printf("\nfreeing up %i%i%i%i.%i%i", i,j,k,l,m,n);
								VirtualFree(subdatabase[i][j][k][l][m][n][0].database , 0, MEM_RELEASE);
								subdatabase[i][j][k][l][m][n][0].database=NULL;
								}

							// white databases
							if(subdatabase[i][j][k][l][m][n][1].database != NULL) 
								{
								printf("\nfreeing up %i%i%i%i.%i%i", i,j,k,l,m,n);
								VirtualFree(subdatabase[i][j][k][l][m][n][1].database , 0, MEM_RELEASE);
								subdatabase[i][j][k][l][m][n][1].database=NULL;
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
