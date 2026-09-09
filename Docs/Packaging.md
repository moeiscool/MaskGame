# Packaging for all five platforms

MaskGame targets Windows, macOS (Apple Silicon), Linux, Android and iOS. This
page covers what each one needs installed and how to build it.

If you have not built the project at all yet, start with [`../SETUP.md`](../SETUP.md)
and come back here.

## The short version

```sh
python3 Scripts/package.py --platform win64
python3 Scripts/package.py --platform mac      --config Shipping
python3 Scripts/package.py --platform linux
python3 Scripts/package.py --platform android
python3 Scripts/package.py --platform ios
```

The script wraps Unreal's `RunUAT BuildCookRun` and fills in the per-platform
flags that are easy to get wrong. Add `--dry-run` to see the exact command it
would run without running it.

| Flag | What it does |
| --- | --- |
| `--config Shipping` | Optimised, no console or debug commands, adds `-distribution` so a store will accept it |
| `--output DIR` | Where the build lands (default `Build/<Platform>`) |
| `--engine PATH` | Where Unreal is, if it is not in the usual place. `UE_ROOT` works too |
| `--clean` | Force a full rebuild |
| `--dry-run` | Print the command and stop |

## What can build what

Apple's toolchain is only licensed for macOS, so a Mac or iOS build needs a Mac.
Everything else is more forgiving.

| Target | Windows host | macOS host | Linux host |
| --- | --- | --- | --- |
| Windows | yes | no | no |
| macOS (Apple Silicon) | no | yes | no |
| Linux | yes, with the cross-compile toolchain | no | yes |
| Android | yes | yes | yes |
| iOS | no | yes | no |

The script refuses a combination that cannot work rather than letting you find
out forty minutes in.

---

## Windows

Nothing beyond what [`../SETUP.md`](../SETUP.md) already had you install:
Visual Studio 2022 with the **Game development with C++** and **Desktop
development with C++** workloads.

```sh
python3 Scripts/package.py --platform win64
```

You get a folder with `MaskGame.exe` in it that needs no engine installed.

---

## macOS (Apple Silicon)

Needs Xcode, and a Mac with an M-series chip.

The script passes `-architecture=arm64`, so you get an Apple Silicon binary
rather than a universal one. That halves both the build time and the size, at
the cost of not running on the Intel Macs Apple stopped selling in 2023. If you
do want a universal build, override it:

```sh
python3 Scripts/package.py --platform mac -- -architecture=arm64+x86_64
```

**Signing.** An unsigned build runs on your own machine but Gatekeeper will
refuse it on anyone else's. To distribute it you need a paid Apple Developer
account and a Developer ID certificate, then to notarise the result. That is
outside what this script does.

---

## Linux

On a Linux host, clang and the standard build tools are enough:

```sh
sudo apt install clang build-essential
python3 Scripts/package.py --platform linux
```

**Cross-compiling from Windows** is supported and is often easier than keeping a
Linux machine around. Install the toolchain that matches your engine version —
for UE 5.5 that is **v23 clang-18.1.0**, from
<https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine>.
Set `LINUX_MULTIARCH_ROOT` to where you unpacked it, restart your terminal, and
the same command works.

---

## Android

This is the fiddliest of the five, because the versions have to match exactly.

### 1. Install Android Studio

From <https://developer.android.com/studio>. You need it for the SDK manager,
not for the IDE.

### 2. Let Unreal install the right SDK and NDK

Unreal ships a script that installs the exact versions it was built against.
**Use it rather than picking versions by hand** — a mismatched NDK is the single
most common cause of an Android build failing in a way that makes no sense.

```
# Windows
"C:\Program Files\Epic Games\UE_5.5\Engine\Extras\Android\SetupAndroid.bat"

# macOS
"/Users/Shared/Epic Games/UE_5.5/Engine/Extras/Android/SetupAndroid.command"

# Linux
~/UnrealEngine/Engine/Extras/Android/SetupAndroid.sh
```

For UE 5.5 that lands on **SDK 34**, **NDK 25.1.8937393**, **JDK 17**. Accept
the licences when it asks.

Restart your terminal afterwards so the environment variables it set are picked
up (`ANDROID_HOME`, `NDKROOT`, `JAVA_HOME`).

### 3. Add the Android platform to the engine

In the Epic Games Launcher: **Library** → the dropdown next to your engine
version → **Options** → tick **Android** under Target Platforms → **Apply**.

### 4. Set your package name

Open `Config/Android/AndroidEngine.ini` and change:

```ini
PackageName=com.maskgame.maskgame
```

to something you own. **This is permanent** — it is the app's identity on the
device and in the Play Store, and it cannot be changed after release.

### 5. Build

```sh
python3 Scripts/package.py --platform android
```

You get an `.apk` in `Build/Android`. Install it with:

```sh
adb install -r Build/Android/MaskGame-arm64.apk
```

### Android notes

- **arm64 only.** `Config/Android/AndroidEngine.ini` turns off the x86_64 slice.
  Every device that can run this is 64-bit ARM, and dropping the other slice
  roughly halves the APK.
- **Vulkan and ES3.1** are both enabled; the engine picks whichever the device
  supports.
- **Lumen and virtual shadow maps are off** on mobile — they are desktop
  features that would either do nothing or cost far too much. The mobile
  renderer uses the older shadow path instead, which is why
  `Config/Android/AndroidEngine.ini` has its own `[/Script/Engine.RendererSettings]`
  block.
- **Minimum SDK 26** (Android 8, 2017).
- **For the Play Store** you want an App Bundle rather than an APK, and a
  Shipping build: `--config Shipping`, then set **Package game data inside .apk**
  off and **Generate bundle (AAB)** on in Project Settings → Android.

---

## iOS

Needs a Mac with Xcode. There is no way around that.

### 1. Xcode and an Apple Developer account

Install Xcode from the App Store. For anything beyond running on the simulator
you need an Apple Developer account (free for on-device testing with a personal
team, $99/year to distribute).

### 2. Add the iOS platform to the engine

Launcher → **Library** → engine dropdown → **Options** → tick **iOS** → Apply.

### 3. Set your bundle identifier

In `Config/IOS/IOSEngine.ini`:

```ini
BundleIdentifier=com.maskgame.maskgame
```

Change it to something matching an App ID in your developer account. Like the
Android package name, this is permanent after release.

### 4. Signing

For a build you will actually install on a device:

```sh
python3 Scripts/package.py --platform ios \
    --signing-identity "Apple Development: you@example.com (XXXXXXXXXX)" \
    --provisioning-profile "MaskGame Development"
```

Find the identity string with `security find-identity -v -p codesigning`.

For a first run it is usually easier to let the editor handle it: open the
project, **Project Settings → Platforms → iOS**, and use the **Import
Certificate** and **Import Provision** buttons, then package from
**Platforms → iOS → Package Project**.

### iOS notes

- **Metal only**, minimum **iOS 16**, A12 and later.
- The same mobile renderer overrides as Android, in `Config/IOS/IOSEngine.ini`.
- **Landscape both ways up**, matching the touch layout.

---

## Touch controls

Android and iOS builds show on-screen controls automatically. To see them on a
desktop build without a touchscreen, open the console and type:

```
MaskGame.Touch.Force 1
```

`0` restores the default (only on touch devices) and `2` forces them off.

The layout is described in [`../README.md`](../README.md#controls). The one
thing worth knowing: the movement stick is *floating* — it appears wherever your
left thumb lands in the left of the screen rather than at a fixed spot, so you
never have to look down to find it.

## Split-screen

Windows, macOS and Linux only. Plug in a second controller and press **A** or
**Start** on it; the screen splits and a second player joins the same cycle.
Up to four.

There is one clock, one set of masks and one notebook: the players share the
three days, and any of them can play the Hymn of Return to take everyone back to
the first dawn.

On mobile it is switched off — one screen, one pair of thumbs.

---

## When packaging fails

**`SetupAndroid` finished but the build says the NDK is missing.** Restart your
terminal, or the whole machine. The script sets environment variables that an
already-open shell will not see.

**iOS: "No code signing identities found".** Open Xcode once, sign in under
**Settings → Accounts**, and let it create a development certificate.

**Linux cross-compile: "Toolchain not found".** `LINUX_MULTIARCH_ROOT` is unset
or points at the wrong version. It must match the engine version exactly.

**The build succeeds but the game is a black screen on device.** Check the
device log (`adb logcat -s UE`) for `LogMaskGame`. The most likely cause is that
the six data tables were never imported, so there is no world to build — see
[`DataTables.md`](DataTables.md).

**Out of disk.** A packaged build plus its intermediates runs to 20–30 GB per
platform. `Build/`, `Binaries/`, `Intermediate/` and `Saved/` are all safe to
delete; they regenerate.
