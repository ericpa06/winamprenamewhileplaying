# Winamp Rename While Playing

A Winamp General Purpose plugin that modifies local-file opens made through the Win32 `CreateFileW` and `CreateFileA` APIs inside the player process. Its default policy adds only `FILE_SHARE_DELETE`, allowing compatible applications to rename or atomically replace a playing track without changing Winamp's playlist entry or stopping playback.

## Important scope

This initial repository implementation hooks **CreateFileW and CreateFileA only**. It does not yet hook `NtCreateFile` or `NtOpenFile`. It is therefore format-independent only for decoders that open their files through those Win32 APIs inside the Winamp process. MP3, OGG, WMA, FLAC and other formats are not individually verified; some decoders may bypass these hooks.

## Build

The GitHub Actions workflow builds the x86 DLL on Windows 2022 and runs Windows tests. Open the Actions tab and download the `gen_fileunlock-x86` artifact from a successful run.

Manual build requirements: Visual Studio 2022 with Desktop C++, CMake 3.21+, and network access for MinHook v1.3.4. Run `build_x86.bat` from a VS Developer Command Prompt.

Output: `build-x86\\bin\\gen_fileunlock.dll`.

## Install

1. Close Winamp/WACUP.
2. Copy `gen_fileunlock.dll` into the player's `Plugins` directory.
3. Restart the player.
4. Open Preferences → Plug-ins → General Purpose and confirm the plugin appears.
5. Play a local test track and try renaming a disposable copy in Explorer.

Use an x86 DLL with 32-bit Winamp. The plugin does not support WACUP x64 unless that host provides a compatible third-party plugin ABI.

## Behavior and limitations

- The default mode adds `FILE_SHARE_DELETE` to eligible `OPEN_EXISTING` read opens. It preserves the caller's other sharing flags.
- This permits rename/move and compatible atomic replacement operations while the existing handle remains open. A rename to a new path does not rewrite Winamp's playlist path.
- It does not preload the file into RAM and does not guarantee consistent playback during in-place writes.
- The hook is process-wide for eligible file opens, not restricted to audio extensions.
- Only Win32 CreateFileA/W opens are intercepted in this version. Native NT opens, out-of-process readers, streams, devices, and custom filesystem paths may not be affected.
- The DLL's Configure action displays the number of adjusted opens.

## Verification status

GitHub Actions compiles the DLL against the Windows SDK and MinHook and runs the file-sharing behavior test. That verifies compilation and Windows sharing semantics, **not** actual Winamp/WACUP loading or decoder-specific behavior. Live integration still needs to be tested on the target player.
