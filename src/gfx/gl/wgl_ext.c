#include "gfx/gl/wgl_ext.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if __GNUC__ || __MINGW32__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

#if defined(__APPLE__)
    #include <dlfcn.h>

static void *AppleGLGetProcAddress(const char *name)
{
    static void *image = NULL;

    if (NULL == image)
        image = dlopen(
            "/System/Library/Frameworks/OpenGL.framework/Versions/"
            "Current/OpenGL",
            RTLD_LAZY);

    return (image ? dlsym(image, name) : NULL);
}
#endif /* __APPLE__ */

#if defined(__sgi) || defined(__sun)
    #include <dlfcn.h>
    #include <stdio.h>

static void *SunGetProcAddress(const GLubyte *name)
{
    static void *h = NULL;
    static void *gpa;

    if (h == NULL) {
        if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL)
            return NULL;
        gpa = dlsym(h, "glXGetProcAddress");
    }

    if (gpa != NULL)
        return ((void *(*)(const GLubyte *))gpa)(name);
    else
        return dlsym(h, (const char *)name);
}
#endif /* __sgi || __sun */

#if defined(_WIN32)

    #ifdef _MSC_VER
        #pragma warning(disable : 4055)
        #pragma warning(disable : 4054)
        #pragma warning(disable : 4996)
    #endif

static int TestPointer(const PROC pTest)
{
    ptrdiff_t iTest;
    if (!pTest)
        return 0;
    iTest = (ptrdiff_t)pTest;

    if (iTest == 1 || iTest == 2 || iTest == 3 || iTest == -1)
        return 0;

    return 1;
}

static PROC WinGetProcAddress(const char *name)
{
    HMODULE glMod = NULL;
    PROC pFunc = wglGetProcAddress((LPCSTR)name);
    if (TestPointer(pFunc)) {
        return pFunc;
    }
    glMod = GetModuleHandleA("OpenGL32.dll");
    return (PROC)GetProcAddress(glMod, (LPCSTR)name);
}

    #define IntGetProcAddress(name) WinGetProcAddress(name)
#else
    #if defined(__APPLE__)
        #define IntGetProcAddress(name) AppleGLGetProcAddress(name)
    #else
        #if defined(__sgi) || defined(__sun)
            #define IntGetProcAddress(name) SunGetProcAddress(name)
        #else /* GLX */
            #include <GL/glx.h>

            #define IntGetProcAddress(name)                                    \
                (*glXGetProcAddressARB)((const GLubyte *)name)
        #endif
    #endif
#endif

int wgl_ext_EXT_swap_control = 0;

/* Extension: EXT_swap_control*/
typedef int(CODEGEN_FUNCPTR *PFN_PTRC_WGLGETSWAPINTERVALEXTPROC)(void);
static int CODEGEN_FUNCPTR Switch_GetSwapIntervalEXT(void);
typedef BOOL(CODEGEN_FUNCPTR *PFN_PTRC_WGLSWAPINTERVALEXTPROC)(int);
static BOOL CODEGEN_FUNCPTR Switch_SwapIntervalEXT(int interval);

/* Extension: EXT_swap_control*/
PFN_PTRC_WGLGETSWAPINTERVALEXTPROC _ptrc_wglGetSwapIntervalEXT =
    Switch_GetSwapIntervalEXT;
PFN_PTRC_WGLSWAPINTERVALEXTPROC _ptrc_wglSwapIntervalEXT =
    Switch_SwapIntervalEXT;

/* Extension: EXT_swap_control*/
static int CODEGEN_FUNCPTR Switch_GetSwapIntervalEXT(void)
{
    _ptrc_wglGetSwapIntervalEXT =
        (PFN_PTRC_WGLGETSWAPINTERVALEXTPROC)IntGetProcAddress(
            "wglGetSwapIntervalEXT");
    return _ptrc_wglGetSwapIntervalEXT();
}

static BOOL CODEGEN_FUNCPTR Switch_SwapIntervalEXT(int interval)
{
    _ptrc_wglSwapIntervalEXT =
        (PFN_PTRC_WGLSWAPINTERVALEXTPROC)IntGetProcAddress(
            "wglSwapIntervalEXT");
    return _ptrc_wglSwapIntervalEXT(interval);
}

static void ClearExtensionVariables(void)
{
    wgl_ext_EXT_swap_control = 0;
}

typedef struct wgl_MapTable_s {
    char *extName;
    int *extVariable;
} wgl_MapTable;

static wgl_MapTable
    g_mappingTable[1]; /*This is intensionally left uninitialized.*/

static void LoadExtByName(const char *extensionName)
{
    wgl_MapTable *tableEnd = &g_mappingTable[1];
    wgl_MapTable *entry = &g_mappingTable[0];
    for (; entry != tableEnd; ++entry) {
        if (strcmp(entry->extName, extensionName) == 0)
            break;
    }

    if (entry != tableEnd)
        *(entry->extVariable) = 1;
}

static void ProcExtsFromExtString(const char *strExtList)
{
    size_t iExtListLen = strlen(strExtList);
    const char *strExtListEnd = strExtList + iExtListLen;
    const char *strCurrPos = strExtList;
    char strWorkBuff[256];

    while (*strCurrPos) {
        /*Get the extension at our position.*/
        int iStrLen = 0;
        const char *strEndStr = strchr(strCurrPos, ' ');
        int iStop = 0;
        if (strEndStr == NULL) {
            strEndStr = strExtListEnd;
            iStop = 1;
        }

        iStrLen = (int)((ptrdiff_t)strEndStr - (ptrdiff_t)strCurrPos);

        if (iStrLen > 255)
            return;

        strncpy(strWorkBuff, strCurrPos, iStrLen);
        strWorkBuff[iStrLen] = '\0';

        LoadExtByName(strWorkBuff);

        strCurrPos = strEndStr + 1;
        if (iStop)
            break;
    }
}

void wgl_CheckExtensions(HDC hdc)
{
    ClearExtensionVariables();

    {
        typedef const char *(CODEGEN_FUNCPTR * MYGETEXTSTRINGPROC)(HDC);
        MYGETEXTSTRINGPROC InternalGetExtensionString =
            (MYGETEXTSTRINGPROC)IntGetProcAddress("wglGetExtensionsStringARB");
        if (!InternalGetExtensionString)
            return;
        ProcExtsFromExtString((const char *)InternalGetExtensionString(hdc));
    }
}

#if __GNUC__ || __MINGW32__
    #pragma GCC diagnostic pop
#endif
