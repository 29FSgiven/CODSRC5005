/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm

    This file is part of CoD4X Plugin Handler source code.

    CoD4X Plugin Handler source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X Plugin Handler source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/

#ifndef BASIC_H
#define BASIC_H

#define __IN_XACMODULE__

// Enums used in this file
#define C 0
#define CPP 1
#define GCC 2
#define BORLAND 3
#define INTEL 4
#define UNKNOWN 5


// Detect the language (C/C++)

#ifdef LANG
    #undef LANG
#endif
#ifdef LANG_NAME
    #undef LANG_NAME
#endif

#ifdef __cplusplus
    #define LANG CPP
    #define LANG_NAME "C++"
#else
    #define LANG C
    #define LANG_NAME "C"
#endif


// Detect the compiler

#ifdef COMPILER
    #undef COMPILER
#endif
#ifdef COMPILER_NAME
    #undef COMPILER_NAME
#endif

#ifdef __GNUC__ 		// GCC
    #define COMPILER GCC
    #define COMPILER_NAME "GCC / G++"
#else
#ifdef __BORLANDC__	// Borland C++
    #define COMPILER BORLAND
    #define COMPILER_NAME "Borland C++"
#else
#ifdef __INTEL_COMPILER	// Intel C Compiler
    #define COMPILER INTEL
    #define COMPILER_NAME "Intel Compiler"
#else				// Some other compiler
    #define COMPILER UNKNOWN
    #define COMPILER_NAME "unknown"
#endif
#endif
#endif


#ifndef __cdecl
    #define __cdecl __attribute__((cdecl))
#endif /*__cdecl*/

#ifdef DLL_EXPORT
    #undef DLL_EXPORT
#endif /*DLL_EXPORT*/

#ifdef PCL_LOCAL
    #undef DLL_EXPORT_LOCAL
#endif /*DLL_EXPORT_LOCAL*/

#ifdef __iceimport
    #undef __iceimport
#endif /*__iceimport*/

#define __iceimport extern __cdecl

#if defined _WIN32 || defined __CYGWIN__        /*Windows*/
    #ifdef __GNUC__                             /*Windows and mingw*/
        #if LANG == CPP                         /*Windows, mingw and c++*/
            #define DLL_EXPORT extern "C" __attribute__ ((dllexport)) __attribute__ ((cdecl))
        #else                                   /*Windows, mingw and c*/
            #define DLL_EXPORT __attribute__ ((dllexport)) __attribute__ ((cdecl))
        #endif /*LANG == CPP*/
    #else                                       /*Windows and msvc*/
        #if LANG == CPP                         /*Windows and msvc and c++*/
            #define DLL_EXPORT extern "C" __declspec(dllexport) __cdecl
        #else                                   /*Windows and msvc and c*/
            #define DLL_EXPORT __declspec(dllexport) __cdecl
        #endif /*LANG == CPP*/
    #endif
    #define PCL_LOCAL
#else                                           /*Unix*/
    #if __GNUC__ >= 4                           /*Unix, modern GCC*/
        #if LANG == CPP                         /*Unix, modern GCC, G++*/
            #define DLL_EXPORT extern "C" __attribute__ ((visibility ("default"))) __attribute__ ((cdecl))
        #else                                   /*Unix, modern GCC, GCC*/
            #define DLL_EXPORT __attribute__ ((visibility ("default"))) __attribute__ ((cdecl))
        #endif /*LANG == CPP*/
        #define PCL_LOCAL  __attribute__ ((visibility ("hidden")))
    #else                                       /*Unix, old GCC*/
        #define PCL
        #define PCL_LOCAL
    #endif
#endif

// We don't need those any more :P
#undef C
#undef CPP
#undef GCC
#undef BORLAND
#undef INTEL
#undef UNKNOWN

void XAC_CreateHash(const char* in, int inlen, char* finalsha65b);
void XAC_CreateFileHash(const char* filepath, char* finalsha65b);
int ACHlp_LoadFile(unsigned char **data, const wchar_t* filename);
bool Sys_VerifyAnticheatSignature(unsigned char* data, unsigned int len, const void* dercert, unsigned int certlen);

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
	ERR_SCRIPT,					// script error occured
	ERR_SCRIPT_DROP,
	ERR_LOCALIZATION,
	ERR_MAPLOADERRORSUMMARY
} errorParm_t;

#endif
