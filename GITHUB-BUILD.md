# Downloading the DLL

Every push to main/master and every manual workflow dispatch builds the x86 plugin on a Windows runner. Open the repository's Actions tab, select the latest successful **Build Winamp File Share Unlocker** run, and download the **gen_fileunlock-x86** artifact. A version tag beginning with v also creates a GitHub Release containing the DLL.

The build is not a substitute for live Winamp/WACUP integration testing. Match DLL bitness to the player; this artifact is x86.
