//
//  simplech_test.m
//  simplech-test
//
//  Created by Denis Zamataev on 24/01/2018.
//  Copyright © 2018 org. All rights reserved.
//

#import <XCTest/XCTest.h>

#define OCCUPIED 0
#define WHITE 1
#define BLACK 2
#define MAN 4
#define KING 8
#define FREE 16
#define CHANGECOLOR 3
#define MAXDEPTH 99
#define MAXMOVES 28




@interface simplech_test : XCTestCase

@end

@implementation simplech_test

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

    /*
                                        (white)
     
     A        B        C        D        E        F        G        H        /
     
            32                  31                30                29       8
     
     28                27                26                25                7
     
            24                  23                22                21       6
     
     20                19                18                17                5
     
            16                  15                14                13       4
     
     12                11                10                9                 3
     
            8                   7                 6                  5       2
     
     4                  3                 2                1                 1
     
                                        (black)
     */

// array's capacity is 33 not 32 in order to exactly match index with standard checkers notation.
// That's why board[0] will always be free.

- (void)logBoard:(int[])board {
    NSMutableArray *strings = [NSMutableArray new];
    for (int i = 0; i < 8; i++) {
        NSString *s = (i%2==0)?@"":@"    ";
        for (int j = 0; j < 4; j++) {
            int idx = (((i + 1) * 4) - j);
            int cell = board [idx];
            NSString *name = [self nameHandleForFigureValue:cell];
            s = [[s stringByAppendingString:@"      "] stringByAppendingString: name];
        }
        [strings addObject:s];
    }
    NSMutableString *output = @"\n".mutableCopy;
    for (int rowIndex = 7; rowIndex >= 0; rowIndex--) {
        [output appendString: strings[rowIndex]];
        [output appendString:@"\n"];
    }
    
    NSLog(@"%@", output);
}

- (NSString *)nameHandleForFigureValue:(int)figure {
    NSString *name = @"?";
    switch (figure) {
        case FREE:
            name = @"[]";
            break;
        case BLACK | MAN:
            name = @"BM";
            break;
        case WHITE | MAN:
            name = @"WM";
            break;
        case BLACK | KING:
            name = @"BK";
            break;
        case WHITE | KING:
            name = @"WK";
            break;
    }
    return name;
}

- (void)testBeaten {
    int board[33];
    
    for (int i = 0; i < 33; i++) {
        board[i] = FREE;
    }
    
    board [18] = WHITE | MAN;
    board [14] = BLACK | MAN;
    
    int result = getmove (board, BLACK, 5.0);
    
    // white man is beaten
    XCTAssertTrue(board[18] == FREE);
    XCTAssertTrue(board[14] == FREE);
    XCTAssertTrue(board[23] == (BLACK|MAN));
    
    NSLog(@"testBeaten getmove resulted in: %i", result);
}

- (void)testDoubleBeatenAndAscend {
    int board[33];
    
    for (int i = 0; i < 33; i++) {
        board[i] = FREE;
    }
    
    board [18] = WHITE | MAN;
    board [26] = WHITE | MAN;
    board [14] = BLACK | MAN;
    
    int result = getmove (board, BLACK, 5.0);
    
    // white man is beaten
    XCTAssertTrue(board[26] == FREE);
    XCTAssertTrue(board[18] == FREE);
    XCTAssertTrue(board[14] == FREE);
    XCTAssertTrue(board[30] == (BLACK|KING));
    
    NSLog(@"testDoubleBeatenAndAscend getmove resulted in: %i", result);
}

- (void)testPlayOne {
    int board[33];
    
    for (int i = 0; i < 33; i++) {
        board[i] = FREE;
    }
    
    board [27] = WHITE | MAN;
    board [31] = WHITE | MAN;
    board [32] = WHITE | MAN;
    board [30] = WHITE | MAN;
    board [25] = WHITE | MAN;
    board [21] = WHITE | MAN;
    
    board [15] = BLACK | MAN;
    board [18] = BLACK | MAN;
    board [11] = BLACK | MAN;
    board [8] = BLACK | MAN;
    board [7] = BLACK | MAN;
    board [6] = BLACK | MAN;
    board [5] = BLACK | MAN;
    
    [self logBoard:board];
    
    int result = getmove (board, BLACK, 5.0);
    [self logBoard:board];
    
    result = getmove (board, WHITE, 5.0);
    [self logBoard:board];
    
    result = getmove (board, BLACK, 5.0);
    [self logBoard:board];
    
    XCTAssertTrue(board[29] == (BLACK|KING));
    
    NSLog(@"testPlayOne getmove resulted in: %i", result);
}

- (void)testPlayTwo {
    int board[33];
    
    for (int i = 0; i < 33; i++) {
        board[i] = FREE;
    }
    
    board [27] = WHITE | MAN;
    board [25] = WHITE | MAN;
    
    board [15] = BLACK | MAN;
    board [18] = BLACK | MAN;
    
    [self logBoard:board];
    
    int result = getmove (board, BLACK, 5.0);
    [self logBoard:board];
    
    result = getmove (board, WHITE, 5.0);
    [self logBoard:board];
    
    result = getmove (board, BLACK, 5.0);
    [self logBoard:board];
    
    XCTAssertTrue(board[22] == (BLACK|MAN)); //so sad that AI does not take win
    
    NSLog(@"testPlayOne getmove resulted in: %i", result);
}

@end


