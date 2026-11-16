# teaspeak-query

ServerQuery client library for **TeaSpeak** / **TeamSpeak 3** servers,
written in modern C++17.

This library provides a thread-safe, cross-platform client for the TeaSpeak
and TeamSpeak 3 ServerQuery protocol. It handles escaping, error bulks,
notification dispatching, and TeaSpeak-specific extensions, so applications
can focus on the logic they actually want to build: bots, dashboards,
moderation tools, integrations. Commands can be issued from any thread; the
library owns a single reader thread and a single dispatcher thread.

## Required Tools

* [CMake 3.15+](https://cmake.org/)
* A C++17-capable compiler:
  * **Windows**: MSVC 2019+ with the `Desktop development with C++` workload
    (provides the MSVC toolset, Windows 10 SDK, and optionally the
    `C++ Clang tools for Windows` component).
  * **Linux / macOS**: Clang 10+ or GCC 9+.
* **On Windows only**: [Visual Studio 2019, 2022, or 2026](https://www.visualstudio.com/)
  with the workload above. The library itself has no runtime dependencies
  beyond the platform's socket API (`ws2_32` on Windows).

## Sources

```bash
# With HTTPS:
git clone https://github.com/0x4D65686469/teaspeak-query.git
# With SSH:
git clone git@github.com:0x4D65686469/teaspeak-query
```

## Building on Windows

Two toolchain paths are supported: **MSVC** (default) and **ClangCL**
(Clang with the MSVC toolchain). The latter requires the
`C++ Clang tools for Windows` component in the Visual Studio Installer.

Both open the same Visual Studio solution generator. Run these from a
regular PowerShell or cmd prompt — CMake will locate the toolchain itself.

```powershell
mkdir build
cd build

# Visual Studio 2019
cmake .. -G "Visual Studio 16 2019" -A x64 -T ClangCL

# Visual Studio 2022
cmake .. -G "Visual Studio 17 2022" -A x64 -T ClangCL

# Visual Studio 2026
cmake .. -G "Visual Studio 18 2026" -A x64 -T ClangCL
```

Drop `-T ClangCL` to use the MSVC compiler instead.

Then build:

```powershell
cmake --build . --config Debug
cmake --build . --config Release
```

Visual Studio generator is multi-config: both Debug and Release are
available from the same build directory. The output goes to
`build\examples\Debug\tq_basic.exe` and
`build\examples\Release\tq_basic.exe` respectively.

### Building with MinGW / LLVM-MinGW (Ninja)

If you prefer a single-config generator and a non-MSVC toolchain, point
CMake at `clang++` from MinGW:

```powershell
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

The output goes to `build\examples\tq_basic.exe`. Use
`-DCMAKE_BUILD_TYPE=Debug` for a debug build. `clang-cl` alone will not
work here — it expects the MSVC SDK, which MinGW does not ship.

## Building on Linux

Make sure Clang (or GCC) and Ninja are installed. On Debian/Ubuntu:

```bash
sudo apt-get install clang cmake ninja-build
```

Then configure and build:

```bash
mkdir build
cd build
cmake .. -G Ninja \
         -DCMAKE_C_COMPILER=clang \
         -DCMAKE_CXX_COMPILER=clang++ \
         -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j
```

The output is `build/examples/tq_basic`.

### Note on `--config` with Ninja

Ninja is a **single-config** generator: `-DCMAKE_BUILD_TYPE` at configure
time decides the configuration, and `cmake --build . --config <name>`
has no effect. To switch between Debug and Release, re-run the `cmake`
configure step with the desired `-DCMAKE_BUILD_TYPE` value, or use two
separate build directories:

```bash
# Debug tree
cmake -S . -B build-debug -G Ninja \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Release tree
cmake -S . -B build-release -G Ninja \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

### Sanitizers

`-DTQ_USE_SANITIZERS=ON` enables ASan + UBSan for the library target.
On a single-config generator (Ninja, Makefile) the flag is applied to
every build. On a multi-config generator (Visual Studio), it is applied
only to the `Debug` configuration. When not passed, it defaults to `ON`
for Debug and `OFF` for Release.

```bash
cmake -S . -B build-asan -G Ninja \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=Debug \
      -DTQ_USE_SANITIZERS=ON
cmake --build build-asan
```

## C++ API

See `include/teaspeak_query/client.hpp` for the complete method list and
documentation.

```cpp
#include <teaspeak_query/teaspeak_query.hpp>

using namespace teaspeak_query;

int main() {
    ServerQuery q("127.0.0.1", 10101);
    q.connect();
    q.login("serveradmin", "password");
    q.select_server(1);
    q.nickname("ExampleBot");

    q.register_notify("textprivate");
    q.on("textmessage", [](const QueryEvent& ev) {
        std::cout << ev.get("invokername") << " -> "
                  << ev.get("msg") << "\n";
    });

    q.disconnect();
}
```

## Acknowledgements

Thanks to [yat.qa](https://yat.qa) for its documentation and the information it has collected.

## License

This Source Code Form is subject to the terms of the **GNU General Public License, version 3 (GPLv3)**.