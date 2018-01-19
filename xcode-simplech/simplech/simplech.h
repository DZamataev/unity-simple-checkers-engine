//
//  simplech.h
//  simplech
//
//  Created by Denis Zamataev on 19/01/2018.
//  Copyright © 2018 org. All rights reserved.
//

#pragma once

#if UNITY_METRO
#define EXPORT_API __declspec(dllexport) __stdcall
#elif UNITY_WIN
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API
#endif

