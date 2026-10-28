# High-speed DMX analyzers for Saleae Logic 2

Six Windows x64 analyzers for 250, 500, 750, 1000, 1250 and 1500 kbaud.
Each has its own name in Logic 2, for example **DMX-512 (1000 kbaud)**.

## More than 512 slots

Packets are delimited by BREAK, not by a 512-slot limit. Slots are numbered
continuously from 1 until the next BREAK. For example, a packet with 2,048 data
slots at 1 Mbaud is displayed as slots 1 through 2048. The analyzer does not
split the packet into universes or insert extra start codes.

This is a non-standard high-speed DMX transport above 250 kbaud and/or 512 slots.
BREAK and MAB retain the upstream physical timing checks: BREAK >= 88 us,
MAB >= 8 us (or >= 4 us with the DMX-1986 option). Data uses 8 bits, no parity,
two stop bits. These checks do not scale with baud rate.

## Get and install the DLLs

1. Open the **Actions** tab and the latest successful **Build and verify analyzers** run.
2. Download **dmx-analyzers-windows-x64-release** from Artifacts and unzip it.
3. Put the six DLLs together in a dedicated folder, for example
   `Documents\Saleae Analyzers`.
4. In Logic 2, open **Edit > Settings > Custom Low Level Analyzers**, select
   that folder, save, and restart Logic.
5. Add the analyzer matching your transmitter's baud rate and select its channel.

Remove old copies of these custom DLLs from
`C:\Program Files\Logic\resources\windows-x64\Analyzers` before restarting;
otherwise the old Debug DLL can still cause a load error. Keep Saleae's built-in
`dmx_512_analyzer.dll`.

The DLLs at the repository root are historical Debug builds. Use the new Release
artifact instead. Do not copy the SDK's `Analyzer.dll` over Logic's own library.

The sample rate must be at least four times the baud rate; use a higher rate
where available. At 1.5 Mbaud the enforced minimum is 6 MS/s.

## Why the old DLLs fail to load

The original DLLs depend on Visual C++ Debug runtimes including
`MSVCP140D.dll`, `VCRUNTIME140D.dll`, `VCRUNTIME140_1D.dll` and
`ucrtbased.dll`. These are not supplied by the normal Visual C++ Redistributable.
The build workflow explicitly produces **Release** DLLs and rejects those Debug
dependencies. A normal current x64 Visual C++ Redistributable may still be
needed for the Release runtime, as with other native Saleae analyzers.

## Build locally on Windows

Install Visual Studio 2019 or newer with **Desktop development with C++**,
CMake 3.15 or newer, and Git.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=OFF
cmake --build build --config Release --parallel
```

The six DLLs are in `build/Analyzers/Release`. Distribute the Release DLLs,
not `build/Analyzers/Debug`.

## Verification

The GitHub workflow builds all six Windows x64 Release DLLs, checks their PE
architecture and runtime imports, loads each DLL and calls its create/destroy
entry points against the pinned Saleae SDK.

On Linux, decoder tests run the actual C++ decoder with Saleae's test harness
against independently generated waveforms. They cover two consecutive packets,
4,096 slots at every baud rate, contiguous slots and inter-slot marks,
every decoded byte and slot number, 1,024 slots with fractional samples per bit, 4 us/8 us MAB
validation, and fixed simulator BREAK/MAB durations. Timeouts catch decoder hangs.

These checks do not replace final validation in Logic 2 with a real capture.

## Source and provenance

Reconstructed from [Saleae's DMX-512 analyzer](https://github.com/saleae/dmx-512-analyzer)
at commit `bfe0c99c80c43ed81f76e70216c93c5676880b68`, under the included MIT
license. The original edited source was unavailable. Baud rate and analyzer name
are compile-time definitions shared by all six builds. The SDK is pinned at
`e3c4b4ffda1c162c024a5913520aed17c6edb238` for reproducible source dependencies.

Changes from upstream: six baud-rate/name variants; consistent 8 us default MAB;
simulator BREAK/MAB use fixed physical durations; slot labels accommodate large
slot indices; explicit Release build workflow and verification.


