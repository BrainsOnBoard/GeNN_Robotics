/*
 * The purpose of this file is to ensure that windows.h is included only once
 * per project and that it is included with the flags we want set appropriately.
 */

#ifdef _WIN32
// the version of the Windows API that we want
#define _WIN32_WINNT _WIN32_WINNT_WIN7

// for a less bloated version of windows.h
#define _WIN32_LEAN_AND_MEAN

// disable the winsock v1 API, which is included by default and conflicts with
// v2 of the API
#define _WINSOCKAPI_

// otherwise windows.h defines min() and max() macros which conflict with
// std::min() - yuck
#define NOMINMAX

#include <windows.h>

// Use the same macro name for Windows and *nix
#ifdef _DEBUG
#define DEBUG
#endif
#endif
