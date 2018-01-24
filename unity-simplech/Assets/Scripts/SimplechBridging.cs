using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;

public class SimplechBridging : MonoBehaviour {

	private const int OCCUPIED = 0;
	private const int WHITE = 1;
	private const int BLACK = 2;
	private const int MAN = 4;
	private const int KING = 8;
	private const int FREE = 16;
	private const int CHANGECOLOR = 3;
	private const int MAXDEPTH = 99;
	private const int MAXMOVES = 28;

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
		coor from,to; /* coordinates of the piece - in 8x8 notation!*/
		coor[] path; /* intermediate path coordinates of the moving piece */
		coor[] del; /* squares whose pieces are deleted after the move */
		int[] delpiece;    /* what is on these squares */
	}

	#if UNITY_IPHONE
	// On iOS plugins are statically linked into the executable, so we have to use __Internal as the library name.
	[DllImport ("__Internal", CallingConvention = CallingConvention.Cdecl)]
	#else
	// Other platforms load plugins dynamically, so pass the name of the plugin's dynamic library.
	[DllImport ("simplech", CallingConvention = CallingConvention.Cdecl)]
	#endif
	private static extern int [,] fillArray(int size);

	#if UNITY_IPHONE
	// On iOS plugins are statically linked into the executable, so we have to use __Internal as the library name.
	[DllImport ("__Internal", CallingConvention = CallingConvention.Cdecl)]
	#else
	// Other platforms load plugins dynamically, so pass the name of the plugin's dynamic library.
	[DllImport ("simplech", CallingConvention = CallingConvention.Cdecl)]
	#endif
	private static extern int sumArray(int[] b);

	#if UNITY_IPHONE
	// On iOS plugins are statically linked into the executable, so we have to use __Internal as the library name.
	[DllImport ("__Internal", CallingConvention = CallingConvention.Cdecl)]
	#else
	// Other platforms load plugins dynamically, so pass the name of the plugin's dynamic library.
	[DllImport ("simplech", CallingConvention = CallingConvention.Cdecl)]
	#endif
	private static extern int getmove(int[] b, int color, double maxtime);

	// Use this for initialization
	void Start () {
		Debug.Log ("[simplech] SimplechBridging behaviour starting");
		ArrayFillTest();
		SumArrayTest();
		GetMoveTest ();
	}

	private void ArrayFillTest() { 
		int size = 512; 
		var start = Time.realtimeSinceStartup; 
		int[,] tab = fillArray(512); 
		Debug.Log("[simplech] test run: array of size " + size.ToString() + " filled in " + (Time.realtimeSinceStartup-start).ToString("f6") + " secs."); 
	}

	private void SumArrayTest() {
		int[] board = new int[8];
		for (int i = 0; i < 8; i++) {
			board[i] = i;
		}
		int res = sumArray (board);
		Debug.Log ("[siplech] test sumArray result: " + res);
	}

	private void GetMoveTest() {

		/*    				
		 					(white)
		 
	A		B		C		D		E		F		G		H	  	/
			
			32				31				30				29		8

	28				27				26				25				7

			24				23				22				21		6

	20				19				18				17				5

			16				15				14				13		4
	
	12				11				10				9				3

			8				7				6				5		2

	4				3				2				1				1

							(black)
		*/

		// array's capacity is 33 not 32 in order to exactly match index with standard checkers notation. 
		// That's why board[0] will always be free.
		int[] board = new int[33];

		for (int i = 0; i < 33; i++) {
			board[i] = FREE;
		}

		board [32] = WHITE | MAN;
		board [29] = WHITE | MAN;
		board [4] = BLACK | MAN;
		board [1] = BLACK | MAN;

		Debug.Log("board was:");
		logBoard (board);

		int result = getmove (board, BLACK, 5.0);

		Debug.Log("getmove resulted in: " + result.ToString() + " where (DRAW 0 WIN 1 LOSS 2 UNKNOWN 3)");

		Debug.Log("board is:");
		logBoard (board);

	}

	private void logBoard(int[] board) {
		string[] strings = new string[8];
		for (int i = 0; i < 8; i++) {
			string s = (i%2==0)?"":"      ";
			for (int j = 0; j < 4; j++) {
				int idx = (((i + 1) * 4) - j);
				int cell = board [idx];
				string name = nameHandleForFigureValue (cell);
				s = s + "      " + name;
			}
			strings [i] = s;
		}
		string output = "";
		for (int rowIndex = 7; rowIndex >= 0; rowIndex--) {
			output += strings[rowIndex] + System.Environment.NewLine;
		}

		Debug.Log (output);
	}

	private string nameHandleForFigureValue(int figure) {
		string name = "?";
		switch (figure) {
		case FREE:
			name = "[   ]";
			break;
		case BLACK | MAN:
			name = "BM";
			break;
		case WHITE | MAN:
			name = "WM";
			break;
		case BLACK | KING:
			name = "BK";
			break;
		case WHITE | KING:
			name = "WK";
			break;
		}
		return name;
	}

	// Update is called once per frame
	void Update () {
		
	}
		
}
