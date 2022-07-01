This is a currently very WIP project to patch Oculus Quest(2) games with a proxy library to allow execution on Pico Neo 3 HMDs.

This repository just generates the proxy .so. 
The resulting .so still has to be patched into oculus games.

Rough workflow:
- Compile .so
- Rename picoAPI.so to libvrapi.so
- Decompile/extract APK
- Replace libvrapi.so inside APK with own one
- Add libpxr_api.so too
- Change AndroidManifest.xml
	- Add `<category android:name="android.intent.category.LAUNCHER"/>` to inent-filter
	- Add `<meta-data android:name="pvr.app.type" android:value="vr"/>` under application 
	- Add `<uses-feature android:name="android.hardware.vr.headtracking" android:required="true" />`
- Repackage/Sign APK

Install APK on device.

Currently by using (Quake2Quest)[https://github.com/DrBeef/Quake2Quest] as a baseline, I got the game to launch with audio and controller vibration. I am not sure if input is working but it very well might be (since one can't see anything it's hard to test).

I know I could just (in the case of Quake2Quest) compile it myself with patches for the Pico API/SDK but this is more a challenge to myself and to learn about lower level stuff. PRs are welcome obviously, I'd like to one day see at least one game fully playable :).

