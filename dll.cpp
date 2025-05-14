#include "ffo.h"
#include "helper/helper.h"

#define DISPLAY3D_DISPATCH(func)                                                                                       \
    FARPROC                fn##func;                                                                                   \
    __declspec(naked) void func##_stub()                                                                               \
    {                                                                                                                  \
        __asm jmp fn##func                                                                                             \
    }

DISPLAY3D_DISPATCH(CreateDisplay3D)
DISPLAY3D_DISPATCH(DestroyDisplay3D)

#define FIND_DISPLAY3D(func) fn##func = GetProcAddress(display3d_module, #func);

static void LoadDisplay3D()
{
    auto display3d_module = LoadLibraryW(L"Display3DO.dll");

    FIND_DISPLAY3D(CreateDisplay3D)
    FIND_DISPLAY3D(DestroyDisplay3D)
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        LoadDisplay3D();
        inject_game();
        helper_instance.patch();
        helper_instance.begin_helper();
        break;
    }

    case DLL_PROCESS_DETACH: {
        helper_instance.end_helper();
        break;
    }

    default:
        break;
    }

    return TRUE;
}
