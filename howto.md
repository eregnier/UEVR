# UEVR Foveated Rendering - How to Compile and Test

## Overview
This guide provides step-by-step instructions for compiling UEVR with foveated rendering support and testing it with PSVR2 on Windows/Steam.

## Prerequisites

### Windows Setup
- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with C++ development tools
- **CMake 3.20+** (https://cmake.org/download/)
- **Git** (https://git-scm.com/download/win)
- **SteamVR** (for PSVR2 compatibility layer)
- **PSVR2** with SteamVR support

### Development Tools
- **Windows SDK 10.0.19041.0** or later
- **DirectX 12** runtime (included with Windows 10/11)
- **Vulkan SDK** (optional, for Vulkan support)

## Step 1: Clone and Setup UEVR

```bash
# Clone the repository
git clone https://github.com/praydog/UEVR.git
cd UEVR

# Initialize submodules
git submodule update --init --recursive

# The foveated rendering changes should already be in place
# Check if the new files exist:
ls src/mods/vr/runtimes/OpenXR_Foveated.hpp
ls src/mods/vr/runtimes/OpenXR_Foveated.cpp
```

## Step 2: Install Dependencies

### Visual Studio Components
1. Open Visual Studio Installer
2. Modify your Visual Studio 2022 installation
3. Install these workloads:
   - **Desktop development with C++**
   - **Windows 10/11 SDK**
   - **MSVC v143 - VS 2022 C++ x64/x86 build tools**

### CMake Configuration
```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64
```

## Step 3: Build UEVR

### From Command Line
```bash
# Build Release version
cmake --build . --config Release

# Build Debug version (for testing)
cmake --build . --config Debug
```

### From Visual Studio
1. Open `UEVR.sln` from the build directory
2. Set configuration to **Release** or **Debug**
3. Build > Build Solution (Ctrl+Shift+B)

### Build Output
The compiled UEVR DLL will be located at:
- `build/Release/UEVR.dll` (Release build)
- `build/Debug/UEVR.dll` (Debug build)

## Step 4: PSVR2 Setup on Windows

### SteamVR Installation
1. Install **Steam** from https://store.steampowered.com
2. Install **SteamVR** from Steam Library
3. Install **PlayStation VR2 PC adapter** drivers:
   - Download from Sony's official website
   - Install PSVR2 PC adapter software

### PSVR2 Configuration
1. Connect PSVR2 to PC via USB-C cable
2. Open **SteamVR**
3. SteamVR should detect PSVR2 automatically
4. Complete room setup in SteamVR
5. Verify tracking works in SteamVR Home

## Step 5: Testing Foveated Rendering

### Game Setup
1. **Launch SteamVR** first
2. **Launch any UE4/UE5 game** through Steam
3. **Inject UEVR** into the game:
   ```
   Method 1: Use UEVR Frontend
   - Run UEVR.exe from build directory
   - Select game process
   - Click "Inject"
   
   Method 2: Manual injection
   - Copy UEVR.dll to game directory
   - Use DLL injector like Extreme Injector
   ```

### Enable Foveated Rendering
1. **Open UEVR Menu** in VR (usually Right Controller Menu button)
2. **Navigate to VR Settings** → "OpenXR Foveated Rendering"
3. **Enable** "Foveated Rendering"
4. **Configure parameters**:
   - **Center Resolution Scale**: 1.0x (start with this)
   - **Peripheral Resolution Scale**: 0.25x (for 75% pixel reduction)
   - **Center Size**: 0.5 (adjust based on preference)
   - **Eye Tracking**: Enable if PSVR2 supports it
5. **Click "Initialize Foveated Rendering"**

### Verification Steps
1. **Check Status Indicator**:
   - Green: Foveated rendering ready
   - Yellow: Available but not initialized
   - Red: Not supported by hardware

2. **Performance Testing**:
   - Enable SteamVR performance overlay
   - Compare FPS before/after enabling foveated rendering
   - Expected improvement: 20-40% FPS gain

3. **Visual Quality Check**:
   - Look at center of view - should be high resolution
   - Check peripheral vision - should show reduced resolution
   - Move eyes/head to verify dynamic adjustment

## Step 6: Troubleshooting

### Common Issues

#### Build Errors
```
Error: CMake not found
Solution: Add CMake to PATH or use full path to cmake.exe

Error: Missing Windows SDK
Solution: Install Windows SDK 10.0.19041.0+ via Visual Studio Installer

Error: OpenXR headers not found
Solution: Ensure git submodules are initialized (`git submodule update --init`)
```

#### PSVR2 Issues
```
Issue: PSVR2 not detected by SteamVR
Solution: 
- Check USB-C connection
- Update PSVR2 PC adapter drivers
- Restart SteamVR
- Try different USB port

Issue: Tracking problems
Solution:
- Ensure good lighting in room
- Check for reflective surfaces
- Recalibrate room setup in SteamVR
```

#### Foveated Rendering Issues
```
Issue: "Not Supported" status
Solution:
- Verify PSVR2 supports quad-view configuration
- Check OpenXR runtime supports foveated rendering
- Try different OpenXR runtime (SteamVR vs Oculus)

Issue: Visual artifacts
Solution:
- Reduce peripheral resolution scale
- Adjust center size parameters
- Disable eye tracking if causing issues
```

### Debug Mode
For debugging, use Debug build:
```bash
cmake --build . --config Debug
```

Enable debug logging in UEVR:
1. Open UEVR menu
2. Go to Debug settings
3. Enable verbose logging
4. Check log files in game directory

## Performance Optimization

### Recommended Settings
- **Center Resolution Scale**: 1.0x - 1.2x
- **Peripheral Resolution Scale**: 0.25x - 0.4x
- **Center Size**: 0.4 - 0.6 (adjust to preference)
- **Eye Tracking**: Enable if supported

### Monitoring Tools
- **SteamVR Performance Graph**: Shows frame timing
- **UEVR Statistics Overlay**: Built-in performance metrics
- **GPU-Z**: Monitor GPU utilization

## Advanced Configuration

### Custom OpenXR Runtime
If using custom OpenXR runtime:
1. Set environment variable: `XR_RUNTIME_JSON=C:\Path\To\runtime.json`
2. Restart SteamVR
3. Verify runtime in SteamVR settings

### Command Line Parameters
```bash
# Launch game with specific settings
UEVR.exe --game "Game.exe" --resolution-scale 1.5 --foveated-enabled
```

## Testing Checklist

- [ ] UEVR compiles successfully
- [ ] PSVR2 detected in SteamVR
- [ ] Game launches in VR mode
- [ ] UEVR menu accessible in VR
- [ ] Foveated rendering enabled
- [ ] Performance improvement observed
- [ ] Visual quality acceptable
- [ ] Eye tracking working (if supported)

## Support

- **UEVR Discord**: https://discord.gg/uevr
- **GitHub Issues**: https://github.com/praydog/UEVR/issues
- **PSVR2 Support**: Sony PlayStation VR2 PC documentation

## Notes
- Foveated rendering requires OpenXR 1.1+
- Some games may need specific compatibility settings
- Performance gains vary by game and hardware
- Always test with latest UEVR version