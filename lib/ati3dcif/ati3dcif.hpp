#pragma once

// declare DLLEXPORT if required
#ifndef DLLEXPORT
#define DLLEXPORT
#endif

// declare WINAPI if required
#ifndef WINAPI
#define WINAPI __stdcall
#endif

// make sure the API is exported, not imported
#define BUILD_AS_DLL

// define custom boolean types so Windows.h isn't required
#define C3D_TRUE 1
#define C3D_FALSE 0

// include actual ATI3DCIF.H from 3D Rage SDK
#include <ati3dcif/ATI3DCIF.h>
