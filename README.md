## How To Build and Run

### 1. PC (Desktop Target)
The desktop build uses CMake to configure your local compilation pipeline using native desktop window contexts (GLFW/GLEW).

1. Open a terminal/command prompt at the project root directory.
2. Execute the convenience automation wrapper script:
   ```bash
   .\script_build_and_run.bat
3. A native window will display both rendering models. Press ESC to cleanly stop execution bounds and release context memory pools.

### 2. Android (Mobile Target)
The Android platform utilizes JNI bindings to bridge JVM lifecycle callbacks over to native C++ execution structures.

1. Launch Android Studio.
2. Open the /android directory as an existing standalone project.
3. Allow Gradle to synchronize dependencies.
4. Execute a Clean Project (Build > Clean Project) to purge any stale structural artifacts.
5. Select a connected physical device or target Virtual Device Emulator, then click Run (Shift + F10).
6. The layout engine will forward the native AAssetManager pointers to enable runtime loading of packaged asset .glsl files.

### 3. Web (WebAssembly / WebGL Target)
Web-facing distribution leverages the Emscripten toolchain (emcc) to target virtual filesystem bounds and render straight onto HTML5 canvases.

1. Navigate to your Emscripten SDK directory:
   ```bash
   cd path/to/your/emsdk
2. Activate the environment variables:
    Windows: .\emsdk_env.bat
    Linux/macOS: ./emsdk_env.bat    
2. Compile the source tree to WebAssembly binaries by executing the target shell script from the project root:
    Windows: .\build_web.bat
    Linux/macOS: ./build_web.sh
3. Launch a local development server:
    cd web ^&^& python -m http.server 8080
4. Open your web browser and navigate to:
    http://localhost:8080/
5. Inspect the developer console to verify successful environment initializations ([INFO] GLFW initialized).
