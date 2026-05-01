# Rotary Encoder on ESP32 Hardware


This repository is for improving C++ knowledge, low-level coding of hardware components.</br>
Documentation is provided in [documentation.md](https://github.com/mawimmer/ESP32-Code/blob/main/docs/documentation.md).
## Style Guide
We are using the following style for all C/C++ files.

```yaml
clang-format -style="{
BasedOnStyle: Google
DerivePointerAlignment: false
BreakBeforeBraces: Attach
IndentWidth: 4
}"
```

## Environment Setup
* Visual Studio Code
```
  Extensions:
  * PlatformIO
  * C/C++ IntelliSense
  * Python Environment (for copy/paste script)
  * Wokwi (optional)
```

#### The workspace structure alongside the *wled* repository:


```
├── ESP32-Code/
│   ├── src/
|   ├── sync.py
|   *
└── WLED/
```

>[!CAUTION]
If you decide for a different stucture, the paths in *sync.py* located in the esp32-code folder have to be adjusted accordingly.

## Workflow

For a cleaner repository, the esp32-code (usermod) was developed on it's distinct repository and copied with **run task** ```sync to wled``` for compiling with *wled*.

>[!IMPORTANT]
Make sure to put ESP32-Code/platformio.override.ini in WLED/ and change the top to your desired env. Add your WLAN credentials or delete those 2 lines if you want to boot with the access-point ([wled-docs](https://kno.wled.ge/basics/getting-started/#:~:text=3.%20Use,embedded%20DNS%20server)). Select your env in platformio to compile and flash your board.

#### <ins>The project comes along with 3 unique envs:</ins>
```
* wokwi_sim
* wled_dev
* mock_compile
```
---
**Select ```wokwi_sim``` if you want to simulate.**

>[!IMPORTANT]
For the simulation to run, you have to implement mocking for your added wled functionalities.
</br>

---
**Select ```wled_dev``` if you want intellisense to recognise the original wled.h.**

>[!NOTE]
If you use a different workspace structure than recommended, you have to adjust the #define paths in all files.
</br>

---
**Select ```mock_compile``` if you want to compile a standalone version and flash it on hardware for testing without wled.**




## Perspective

Goal is to provide a library that can be used as usermod in wled but also as standalone with interfaces.

>[!NOTE]
Right now it will throw compiler and/or linker errors without wled/wled_mock.


