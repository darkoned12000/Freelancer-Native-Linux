// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#include "DALib.h"
#include "dacom.h"

#if !defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
#include "alchemy.h"
#include "soundmanager.h"
#include "soundstreamer.h"
#include "system.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
#include <io.h>
#include <unknwn.h>
#else
#include "linux_port/platform.h"
#include <readfile.h>
#endif

namespace
{
#if !defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
    // Retail .text:065c15d0 (FUN_065c15d0) -- cdecl debug print: a NULL format does nothing;
    // otherwise vsprintf into a 0x100-byte stack buffer (unbounded, as retail) and
    // OutputDebugStringA. Only caller: DALib::Startup's DACOM/INI failure path.
    void DebugPrint(const char* Format, ...)
    {
        if (Format == nullptr)
            return;
        char Buffer[0x100];
        va_list Args;
        va_start(Args, Format);
        vsprintf(Buffer, Format, Args);
        va_end(Args);
        OutputDebugStringA(Buffer);
    }
#endif

    // SystemContainer uses the extended descriptor; other factories consume its prefix.
    struct FNamedAggDesc
    {
        unsigned long Size;
        const char* Name;
        void* Outer;
        void* InnerOut;
        const char* Extra;
    };

    // Kept across calls for Shutdown's trailing-reference checks.
#if defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
    [[maybe_unused]] unsigned long GLastReleaseCount = 0;
#else
    unsigned long GLastReleaseCount = 0;
#endif
}

#if !defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
// Retail .data:065ca2ac -- DALib's own module handle, stored by DllMain (.text:065c5d4c) and
// passed to GetModuleFileNameA by Startup (.text:065c11b0).
static HMODULE GModule = nullptr;
#else
// Linux core-only TU: module handle not needed.
#endif

// Retail .data:065ca0b0 / 065ca0b4 -- both start NULL; the host installs its handlers before
// DALib::Startup (freelancer.exe WinMain .text:005b30e8 / 005b30f4).
FatalErrorFnType FatalErrorFn = nullptr;
CriticalWarningFnType CriticalWarningFn = nullptr;

ICOManager* DALib::Dacom = nullptr;
IEngine* DALib::Engine = nullptr;
IEngine2* DALib::Engine2 = nullptr;
IRenderer2* DALib::Renderer = nullptr;
IAnimation2* DALib::Anim = nullptr;
IHardpoint* DALib::Hardpoint = nullptr;
IDeformable* DALib::Deform = nullptr;
IChannel* DALib::Channel = nullptr;
IRenderPipeline* DALib::Pipeline = nullptr;
IRPDraw* DALib::RPDraw = nullptr;
IRPIndexBuffer* DALib::RPIndexBuffer = nullptr;
IRPVertexBuffer* DALib::RPVertexBuffer = nullptr;
IFileSystem* DALib::Data = nullptr;
ISoundManager* DALib::Sound = nullptr;
IStreamer2* DALib::Streamer = nullptr;
ITextureLibrary2* DALib::TextureLib = nullptr;
IMaterialLibrary* DALib::MaterialLib = nullptr;
IMaterialBatcher2* DALib::MaterialBatcher = nullptr;
IVertexBufferManager* DALib::VertexBuffer = nullptr;
IAlchemy* DALib::Alchemy = nullptr;
IFxRuntime* DALib::FxRuntime = nullptr;
IFxEffectLibrary* DALib::FxEffectLibrary = nullptr;
IMatAnimLibrary* DALib::MatAnimLibrary = nullptr;
IVMeshLibrary* DALib::VMeshLibrary = nullptr;

namespace
{
    // One `if (p) { LastRelease = p->Release(); p = 0; }` block of DALib's teardown
    // (.text:065c16d5.., e.g. Channel .text:065c16f1-1705). The count is stored even when 0.
    template <typename T>
    void ReleaseGlobal(T*& Global)
    {
        if (Global != nullptr)
        {
            GLastReleaseCount = ((IDAComponent*)Global)->Release();
            Global = nullptr;
        }
    }
}

#if !defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
// Retail .text:065c1160 DALib::Startup(HWND, const char*).
bool __cdecl DALib::Startup(HWND Hwnd, const char* IniPath)
{
    Dacom = DACOM_Acquire();
    if (Dacom == nullptr)
    {
        DebugPrint("ERROR: creating DACOM with INI file\n"); // .text:065c15aa
        return false;
    }

    bool bResult = false;
    ISystemContainer* Container = nullptr;

    // 0x11a4: CreateInstance(AggDesc{0x1C, "SystemContainer", (void*)IniPath}) into Container.
    FSystemContainerAggDesc AggDesc = {};
    AggDesc.Size = 0x1c;
    AggDesc.ComponentName = "SystemContainer";
    AggDesc.IniPath = (void*)IniPath;
    if (Dacom->CreateInstance((AGGDESC*)(&AggDesc), (void**)(&Container)) != GR_OK)
    {
        DebugPrint("ERROR: unable to create system\n"); // .text:065c15cf
        goto Exit;
    }

    // 0x11c9..0x1278: ISystemContainer::LoadSystemComponents (slot +0x0C) from a 0x18-byte
    // on-stack component table ("Engine"/IID_IEngine, "Hardpoint"/IID_IHardpoint,
    // "Animation"/IID_IAnimation, "Channel"/IID_IChannel, "Deformable"/IID_IDeformable) into
    // the DALib globals in that order.
    {
        FLoadComponentDesc Components[5];
        Components[0].ComponentName = "Engine";
        Components[0].Iid = IID_IEngine;
        Components[0].Out = (void**)(&Engine);
        Components[1].ComponentName = "Hardpoint";
        Components[1].Iid = IID_IHardpoint;
        Components[1].Out = (void**)(&Hardpoint);
        Components[2].ComponentName = "Animation";
        Components[2].Iid = IID_IAnimation;
        Components[2].Out = (void**)(&Anim);
        Components[3].ComponentName = "Channel";
        Components[3].Iid = IID_IChannel;
        Components[3].Out = (void**)(&Channel);
        Components[4].ComponentName = "Deformable";
        Components[4].Iid = IID_IDeformable;
        Components[4].Out = (void**)(&Deform);
        if (((GENRESULT(__stdcall*)(ISystemContainer*, FLoadComponentDesc*, void*,
                                    unsigned long))Container->vtable[3])(Container, Components,
                                                                         nullptr, 5) != GR_OK)
        {
            DebugPrint("ERROR: loading critical components\n"); // .text:065c15f7
            goto Exit;
        }
    }

    // 0x1292..0x138b: ten QIs off the container -- the six render/asset interfaces
    // (IRenderPipeline8B, ITextureLibrary2, IMaterialLibrary, IMaterialBatcher2,
    // IVertexBufferManager, IRPDraw, in that order) with a FDUMP on any failure and a bail at
    // 0x1330/0x138b, then IAlchemy/IFxRuntime/IMatAnimLibrary/IVMeshLibrary whose failures are
    // only recorded and do not bail.
    {
        const char* const RenderNames[6] = {Iids::RenderPipeline8B, Iids::TextureLibrary2,
                                            Iids::MaterialLibrary, Iids::MaterialBatcher2,
                                            Iids::VertexBufferManager, Iids::RPDraw};
        void** const RenderSlots[6] = {(void**)(&Pipeline),    (void**)(&TextureLib),
                                       (void**)(&MaterialLib), (void**)(&MaterialBatcher),
                                       (void**)(&VertexBuffer), (void**)(&RPDraw)};
        for (int Index = 0; Index < 6; ++Index)
        {
            if (((HRESULT(__stdcall*)(ISystemContainer*, const char*, void**))Container
                     ->vtable[0])(Container, RenderNames[Index], RenderSlots[Index]) != S_OK)
            {
                FDUMP(0x100002, "ERROR: unable to acquire render interface %s\n",
                      RenderNames[Index]);
                goto Exit;
            }
        }
        // 0x133a..0x138b: IAlchemy, IFxRuntime, IMatAnimLibrary, IVMeshLibrary (in this order) --
        // each failure only records a flag; the bail at 0x138b is this flag, never the render one.
        {
            bool bOptionalMissing = false;
            const char* const OptionalNames[4] = {Iids::Alchemy, Iids::FxRuntime,
                                                  Iids::MatAnimLibrary, Iids::VMeshLibrary};
            void** const OptionalSlots[4] = {(void**)(&Alchemy), (void**)(&FxRuntime),
                                             (void**)(&MatAnimLibrary), (void**)(&VMeshLibrary)};
            for (int Index = 0; Index < 4; ++Index)
            {
                if (((HRESULT(__stdcall*)(ISystemContainer*, const char*, void**))Container
                         ->vtable[0])(Container, OptionalNames[Index], OptionalSlots[Index]) != S_OK)
                    bOptionalMissing = true;
            }
            if (bOptionalMissing)
            {
                FDUMP(0x100002, "ERROR: unable to acquire alchemy interface\n"); // .text:065c146d
                goto Exit;
            }
        }
    }

    // 0x1390..0x1414: QIs for ISoundManager (IID_ISoundManager8) and IStreamer2 (IID_IStreamer2)
    // -- the FDUMP prints the name when a slot is missing, but either failure is otherwise
    // ignored (the code falls through and uses the globals anyway).
    if (((HRESULT(__stdcall*)(ISystemContainer*, const char*, void**))Container->vtable[0])(
            Container, Iids::SoundManager8, (void**)(&Sound)) != S_OK ||
        ((HRESULT(__stdcall*)(ISystemContainer*, const char*, void**))Container->vtable[0])(
            Container, Iids::Streamer2, (void**)(&Streamer)) != S_OK)
    {
        FDUMP(0x100002, "ERROR: unable to acquire sound interface %s\n",
              Sound != nullptr ? Iids::Streamer2 : Iids::SoundManager8);
    }

    // 0x1419..0x143a: ISoundManager::Prepare (slot +0xA4 = +0x298) -- a prepare/startup, not
    // the ISource playback seek. 0x113 non-zero parameter, null third. On failure the engine
    // is torn down: slot +0x28, both QIs released-and-cleared, slot +0xAC.
    if (((GENRESULT(__stdcall*)(ISoundManager*, HWND, unsigned long,
                                void*))Sound->vtable[0xA4 / 4])(Sound, Hwnd, 0x113, nullptr) != GR_OK)
    {
        DebugPrint("ERROR: unable to startup audio system\n"); // .text:065c1507
        ((void(__stdcall*)(IEngine*))Engine->vtable[0x28 / 4])(Engine);
        ((unsigned long(__stdcall*)(IEngine*))Engine->vtable[2])(Engine);
        Engine = nullptr;
        ((unsigned long(__stdcall*)(IEngine2*))Engine2->vtable[2])(Engine2);
        Engine2 = nullptr;
        ((GENRESULT(__stdcall*)(ISoundManager*))Sound->vtable[0xAC / 4])(Sound);
        goto Exit;
    }

    // 0x1450..0x1467: ISoundManager::GetDirectSound (slot +0xBC = +0x2F0) feeds IStreamer2::Load
    // (slot +0x0C). The streamer is not shut down on a Load failure (0x1467 just continues).
    {
        void* DirectSound = nullptr;
        if (((HRESULT(__stdcall*)(ISoundManager*, void**))Sound->vtable[0xBC / 4])(Sound,
                                                                                   &DirectSound) ==
            S_OK)
        {
            FStreamerDesc StreamerDesc = {};
            StreamerDesc.Size = 0x18;
            StreamerDesc.DirectSound = DirectSound;
            StreamerDesc.BufferBytes = 0x10f40;
            StreamerDesc.ChannelCount = 16;
            StreamerDesc.Flags = 3;
            StreamerDesc.SampleRate = 22050;
            if (((int(__stdcall*)(IStreamer2*, FStreamerDesc*))Streamer->vtable[3])(
                    Streamer, &StreamerDesc) < 0)
                FDUMP(0x100002, "ERROR: unable to startup streamer\n"); // .text:065c1533
        }
    }

    // 0x1467..0x148a: QIs for IEngine2 ("Engine") and IRenderer2 ("Renderer"), then
    // IEngine::load_engine_components (slot +0x0C) with the system container.
    if (((HRESULT(__stdcall*)(ISystemContainer*, const char*, void**))Container->vtable[0])(
            Container, "Engine", (void**)(&Engine2)) != S_OK ||
        ((HRESULT(__stdcall*)(ISystemContainer*, const char*, void**))Container->vtable[0])(
            Container, "Renderer", (void**)(&Renderer)) != S_OK)
    {
        // .text:065c153c -- the QIs bailed before bResult went true, so the cleanup does not
        // run ReleaseGlobal(Engine2) and LoadSystemComponents' globals (Hardpoint/Anim/Channel/
        // Deform) leak.
        FDUMP(0x100002, "WARNING: engine or renderer does not provide IEngine2 or IRenderer2\n");
        goto Exit;
    }
    ((GENRESULT(__stdcall*)(IEngine*, ISystemContainer*))Engine->vtable[3])(Engine, Container);
    if (((HRESULT(__stdcall*)(ISystemContainer*, const char*, void**))Container->vtable[0])(
            Container, Iids::FxEffectLibrary, (void**)(&FxEffectLibrary)) != S_OK)
        FxEffectLibrary = nullptr;
    if (FxEffectLibrary != nullptr)
        ((void(__stdcall*)(IFxEffectLibrary*))FxEffectLibrary->vtable[0x20 / 4])(FxEffectLibrary);

    bResult = true;

Exit:
    if (!bResult)
    {
        ReleaseGlobal(Deform);
        ReleaseGlobal(Hardpoint);
        ReleaseGlobal(Anim);
        ReleaseGlobal(Channel);
        ReleaseGlobal(Engine);
    }
    if (Container != nullptr)
        Container->Release();

    return bResult;
}
#else
// M8.7 Linux core-only build: upstream DALib::Startup (SystemContainer/engine/audio/render/
// DirectPlay subsystems) is not yet ported. Acquire DACOM for upstream OpenData/data-path use.
bool __cdecl DALib::Startup(HWND /*Hwnd*/, const char* /*IniPath*/)
{
    Dacom = DACOM_Acquire();
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "DALib::Startup (M8.7 upstream core-only TU): subsystem init pending; DACOM acquired for data-path bring-up");
    return Dacom != nullptr;
}
#endif
// Retail .text:065c1610 DALib::OpenData -- 0x34-byte "FileSystem"/"DOS" descriptor, result
// written straight into DALib::Data; neither Dacom nor the result is checked.
void __cdecl DALib::OpenData(const char* DataPath)
{
    FFileSystemAggDesc Desc = {};
    static_assert(sizeof(FFileSystemAggDesc) == 0x34, "FFileSystemAggDesc must be 0x34 bytes");
    Desc.Size = 0x34;
    Desc.ComponentName = "FileSystem";
    Desc.BackendName = "DOS";
    Desc.DataPath = DataPath;
    Desc.Field10 = 0x80000000; // GENERIC_READ
    Desc.Field14 = 1;          // FILE_SHARE_READ
    Desc.Field1C = 3;          // OPEN_EXISTING
    Desc.Field20 = 0x08000080;
    Desc.Field30 = 0xFFFFFFFF;
    Dacom->CreateInstance((AGGDESC*)(&Desc), (void**)(&Data));
}

// Retail .text:065c1690 DALib::CloseData.
void __cdecl DALib::CloseData()
{
    ReleaseGlobal(Data);
}

#if !defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
// Retail .text:065c16b0 DALib::Shutdown. DALib::Data is not touched (CloseData is the host's
// job); MatAnimLibrary and VMeshLibrary are neither released nor cleared.
void __cdecl DALib::Shutdown(HWND /*Hwnd*/)
{
    OutputDebugStringA("DALib::Shutdown () ENTER\n");

    if (FxEffectLibrary != nullptr)
        FxEffectLibrary->ClearEffects(); // slot +0x24
    IRenderPipeline* const SavedPipeline = Pipeline; // EBX, .text:065c16d8
    ReleaseGlobal(FxEffectLibrary);
    ReleaseGlobal(Channel);
    ReleaseGlobal(Anim);
    ReleaseGlobal(Renderer);
    ReleaseGlobal(Hardpoint);
    ReleaseGlobal(Deform);
    ReleaseGlobal(Engine2);
    ReleaseGlobal(Engine);

    if (GLastReleaseCount != 0)
    {
        FDUMP(0x100002, "ERROR: Release() of component Engine did not return 0\n"); // .text:065c1570
    }
    else if (Pipeline != nullptr)
    {
        // 0x1728..0x1783: FDUMP with Release() of Pipeline in the argument list (EAX) -- so the
        // call is made regardless of the pointer. The zero case leaves the pointer in EBX; the
        // non-zero case clears it and nulls Pipeline.
        if (Pipeline->Release() != 0)
            Pipeline = nullptr;
    }

    const unsigned long VertexBufferRefs = ((IDAComponent*)VertexBuffer)->Release();
    if (VertexBufferRefs != 0)
    {
        // 0x17c5-17f0: stack probe, debug report, stack check (SEH tail 0x17e0-17ef).
        char Buffer[0x400];
        sprintf(Buffer, "ERROR: Ref count for %s is: %d (it should be 0)", "IVertexBufferManager",
                VertexBufferRefs);
        OutputDebugStringA(Buffer); // .text:065c1595
    }

    ReleaseGlobal(MaterialBatcher);
    ReleaseGlobal(MaterialLib);
    ReleaseGlobal(TextureLib);
    ReleaseGlobal(Sound);
    ReleaseGlobal(Streamer);
    ReleaseGlobal(Alchemy);
    ReleaseGlobal(RPDraw);
    ReleaseGlobal(FxRuntime);

    // 0x1817..0x183c: Pipeline slot 4 (+0x10) under the outer check. The debug-build branch
    // 0x1840-184e (FDUMP when SavedPipeline was null) is compiled out of retail.
    if (Pipeline != nullptr)
        ((void(__stdcall*)(IRenderPipeline*))Pipeline->vtable[4])(Pipeline);

    // 0x184f..0x1873: ICOManager::Shutdown (slot +0x24) on the tail of the "saved Pipeline was
    // non-null" path, then Dacom is cleared.
    if (SavedPipeline != nullptr && Dacom != nullptr)
    {
        Dacom->Shutdown(); // slot +0x24
        Dacom = nullptr;
    }

    OutputDebugStringA("DALib::Shutdown () EXIT\n");
}
#else
// M8.7 Linux core-only build: full subsystem teardown is not yet ported.
// Data lifecycle is managed explicitly via CloseData in bring-up flow.
void __cdecl DALib::Shutdown(HWND /*Hwnd*/)
{
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "DALib::Shutdown (M8.7 upstream core-only TU): full subsystem teardown pending");
}
#endif
#if !defined(FL_LINUX_PORT_DALIB_CORE_ONLY)
// Retail .text:065c5d40 DllMain: on DLL_PROCESS_ATTACH (reason 1) stash the module handle in
// .data:065ca2ac and opt out of thread notifications; every reason returns 1.
BOOL WINAPI DllMain(HMODULE Module, DWORD Reason, LPVOID)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        GModule = Module;
        DisableThreadLibraryCalls(Module);
    }
    return TRUE;
}
#endif
