# vcxproj Dependency Graph

Generated from `src/**/*.vcxproj` by `linux/tools/generate_vcxproj_dep_graph.py`.

Sources: `<ProjectReference>` includes and `<AdditionalDependencies>` `.lib` tokens (excluding `%(...)` placeholders). Names are normalized (basename, path/macro/quote stripping for `.lib` tokens, semicolon-delimited parsing) and canonicalized to known project names case-insensitively where possible. This is a **build/reference hint graph**, not a complete runtime/interface or COM/DACOM dependency model.

| Project | Path | Project refs | Additional deps (.lib) | All deps (union) | Depended on by |
|---|---|---|---|---|---|
| `alchemy` | `alchemy/alchemy.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `Common` | `Common/Common.vcxproj` | `DALib`, `dacom`, `qhull` | - | `DALib`, `dacom`, `qhull` | `Content`, `FLServer`, `RemoteClient`, `RemoteServer`, `Server`, `freelancer`, `rpclocal` |
| `Content` | `Content/Content.vcxproj` | `Common`, `Server` | `Common`, `Server`, `dacom` | `Common`, `Server`, `dacom` | - |
| `dacom` | `dacom/dacom.vcxproj` | - | - | - | `Common`, `Content`, `DALib`, `Deformable2`, `EngBase`, `FLServer`, `RP8`, `RP9`, `ReadFile`, `RemoteClient`, `RemoteServer`, `RendComp`, `Server`, `Shading`, `SoundManager`, `SoundStreamer`, `Thorn`, `alchemy`, `flmaterials`, `freelancer`, `luaprofile`, `rpclocal`, `system`, `x86math`, `ximage` |
| `DALib` | `DALib/DALib.vcxproj` | `dacom`, `zlib` | `dacom`, `winmm`, `zlib` | `dacom`, `winmm`, `zlib` | `Common`, `FLServer`, `RemoteClient`, `RemoteServer`, `Server`, `freelancer` |
| `Deformable2` | `Deformable2/Deformable2.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `EngBase` | `EngBase/EngBase.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `EquipResources` | `Resources/EquipResources/EquipResources.vcxproj` | - | - | - | - |
| `flmaterials` | `flmaterials/flmaterials.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `FLServer` | `FLServer/FLServer.vcxproj` | `Common`, `DALib`, `RemoteClient`, `Server`, `dacom`, `serverresources` | `Common`, `DALib`, `RemoteClient`, `Server`, `dacom`, `imm32`, `winmm` | `Common`, `DALib`, `RemoteClient`, `Server`, `dacom`, `imm32`, `serverresources`, `winmm` | - |
| `freelancer` | `freelancer/freelancer.vcxproj` | `Common`, `DALib`, `dacom`, `imeui` | `Common`, `DALib`, `advapi32`, `dacom`, `gdi32`, `imeui`, `ole32`, `shell32`, `user32`, `winmm` | `Common`, `DALib`, `advapi32`, `dacom`, `gdi32`, `imeui`, `ole32`, `shell32`, `user32`, `winmm` | `RemoteServer` |
| `imeui` | `imeui/imeui.vcxproj` | - | `advapi32`, `gdi32`, `imm32`, `ole32`, `user32` | `advapi32`, `gdi32`, `imm32`, `ole32`, `user32` | `freelancer` |
| `InfoCards` | `Resources/InfoCards/InfoCards.vcxproj` | - | - | - | - |
| `luaprofile` | `luaprofile/luaprofile.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `MemoryDiagnostics` | `MemoryDiagnostics/MemoryDiagnostics.vcxproj` | - | `advapi32`, `comdlg32`, `gdi32`, `ole32`, `psapi`, `shell32`, `user32` | `advapi32`, `comdlg32`, `gdi32`, `ole32`, `psapi`, `shell32`, `user32` | - |
| `MiscText` | `Resources/MiscText/MiscText.vcxproj` | - | - | - | - |
| `MiscTextInfo2` | `Resources/MiscTextInfo2/MiscTextInfo2.vcxproj` | - | - | - | - |
| `movie` | `movie/movie.vcxproj` | - | `ole32`, `user32` | `ole32`, `user32` | - |
| `NameResources` | `Resources/NameResources/NameResources.vcxproj` | - | - | - | - |
| `OfferBribeResources` | `Resources/OfferBribeResources/OfferBribeResources.vcxproj` | - | - | - | - |
| `qhull` | `qhull/qhull.vcxproj` | - | - | - | `Common` |
| `ReadFile` | `ReadFile/ReadFile.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `RemoteClient` | `RemoteClient/RemoteClient.vcxproj` | `Server`, `dacom` | `Common`, `DALib`, `Server`, `dacom`, `kernel32`, `rpclocal`, `user32` | `Common`, `DALib`, `Server`, `dacom`, `kernel32`, `rpclocal`, `user32` | `FLServer` |
| `RemoteServer` | `RemoteServer/RemoteServer.vcxproj` | `dacom`, `freelancer` | `Common`, `DALib`, `Server`, `dacom`, `freelancer`, `kernel32`, `rpclocal`, `user32` | `Common`, `DALib`, `Server`, `dacom`, `freelancer`, `kernel32`, `rpclocal`, `user32` | - |
| `RendComp` | `RendComp/RendComp.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `resources` | `Resources/resources/resources.vcxproj` | - | - | - | - |
| `RP8` | `RP8/RP8.vcxproj` | `dacom` | `d3d8`, `d3dx8`, `dacom`, `ddraw`, `legacy_stdio_definitions` | `d3d8`, `d3dx8`, `dacom`, `ddraw`, `legacy_stdio_definitions` | - |
| `RP9` | `RP9/RP9.vcxproj` | `dacom` | `DxErr`, `d3d9`, `d3dx9`, `dacom`, `ddraw`, `legacy_stdio_definitions`, `version` | `DxErr`, `d3d9`, `d3dx9`, `dacom`, `ddraw`, `legacy_stdio_definitions`, `version` | - |
| `rpclocal` | `rpclocal/rpclocal.vcxproj` | `Common`, `dacom` | `Common`, `kernel32`, `user32` | `Common`, `dacom`, `kernel32`, `user32` | `RemoteClient`, `RemoteServer` |
| `Server` | `Server/Server.vcxproj` | `DALib`, `dacom` | `Common`, `advapi32`, `comdlg32`, `gdi32`, `kernel32`, `odbc32`, `odbccp32`, `ole32`, `oleaut32`, `shell32`, `user32`, `uuid`, `winspool` | `Common`, `DALib`, `advapi32`, `comdlg32`, `dacom`, `gdi32`, `kernel32`, `odbc32`, `odbccp32`, `ole32`, `oleaut32`, `shell32`, `user32`, `uuid`, `winspool` | `Content`, `FLServer`, `RemoteClient`, `RemoteServer` |
| `serverresources` | `Resources/serverresources/serverresources.vcxproj` | - | - | - | `FLServer` |
| `Shading` | `Shading/Shading.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `SoundManager` | `SoundManager/SoundManager.vcxproj` | `dacom` | `dacom`, `dsound`, `msacm32`, `ole32`, `winmm` | `dacom`, `dsound`, `msacm32`, `ole32`, `winmm` | - |
| `SoundStreamer` | `SoundStreamer/SoundStreamer.vcxproj` | `dacom` | `dacom`, `dsound`, `msacm32`, `ole32`, `winmm` | `dacom`, `dsound`, `msacm32`, `ole32`, `winmm` | - |
| `system` | `system/system.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `Thorn` | `Thorn/Thorn.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `x86math` | `x86math/x86math.vcxproj` | `dacom` | `dacom` | `dacom` | - |
| `ximage` | `ximage/ximage.vcxproj` | `dacom` | `dacom`, `user32` | `dacom`, `user32` | - |
| `zlib` | `zlib/zlib.vcxproj` | - | - | - | `DALib` |
