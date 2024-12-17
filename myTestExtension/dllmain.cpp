// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdio.h>
#include <type_traits>
#include <concepts>
#include <utility>
#include <format>
#include <string>

#include "../inc/detours.h"

#pragma region detour dark magic
enum class CallConv {
    Cdecl,
    Stdcall,
};

template <auto OldF, auto NewF, CallConv Conv>
//requires std::same_as<decltype(OldF), decltype(NewF)>
struct detour {
    static detour<OldF, NewF, Conv> instance;

    decltype(OldF) real_func = OldF;

    detour() {}

    detour(const detour &) = delete;
    detour(detour &&) = delete;
    detour &operator=(const detour &) = delete;
    detour &operator=(detour &&) = delete;

    static void attach() {
        DetourAttach(reinterpret_cast<PVOID*>(&instance.real_func), reinterpret_cast<PVOID>((decltype(OldF))&_hook));
    }

    static void detach() {
        DetourDetach(reinterpret_cast<PVOID*>(&instance.real_func), reinterpret_cast<PVOID>((decltype(OldF))&_hook));
    }

    template <typename... Args>
        requires (Conv == CallConv::Cdecl)
    static auto __cdecl _hook(Args ... args) {
        return NewF(instance.real_func, args...);
    }

    template <typename... Args>
        requires (Conv == CallConv::Stdcall)
    static auto __stdcall _hook(Args ... args) {
        return NewF(instance.real_func, args...);
    }
};

template <auto OldF, auto NewF, CallConv Conv>
detour<OldF, NewF, Conv> detour<OldF, NewF, Conv>::instance{};
#pragma endregion

using det_create_mutex = detour<&CreateMutexW, [](auto real_func, LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName) -> HANDLE {
    std::wstring new_name = std::format(L"{}_{:X}", lpName, GetCurrentProcessId());
    std::wstring msg = std::format(L"Fixed: {} => {}", lpName, new_name);
    MessageBox(nullptr, msg.c_str(), L"myTestExtesnion: MyCreateMutexW", MB_OK);

    return real_func(lpMutexAttributes, bInitialOwner, new_name.c_str());
}, CallConv::Cdecl>;

extern "C" void __declspec(dllimport) TestApi(LPCWSTR s);

using det_testapi = detour<&TestApi, [](auto real_func, LPCWSTR s) -> void {
    //MessageBox(nullptr, s, L"myTestExtesnion: TestApi", MB_OK);
    std::wstring msg = std::format(L"Fixed: {}", s);
    return real_func(msg.c_str());
}, CallConv::Stdcall>;

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        //MessageBox(nullptr, L"dll was attached!", L"TestDllExtension", MB_OK);
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        det_create_mutex::attach();
        det_testapi::attach();
        DetourTransactionCommit();
        break;
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        det_create_mutex::detach();
        det_testapi::attach();
        DetourTransactionCommit();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}


