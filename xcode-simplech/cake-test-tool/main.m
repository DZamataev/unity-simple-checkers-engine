//
//  main.m
//  cake-test-tool
//
//  Created by Denis Zamataev on 26/01/2018.
//  Copyright © 2018 org. All rights reserved.
//

#import <Foundation/Foundation.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"Executing testcake...");
        
        int result = testcake_main();
        
        NSLog(@"Finished executing testcake.");
    }
    return 0;
}
