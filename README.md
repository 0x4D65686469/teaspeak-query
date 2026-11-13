# teaspeak-query

ServerQuery client library for **TeaSpeak** / **TeamSpeak 3** servers,
written in modern C++17.

**Developer:** [Mehdi](https://github.com/0x4D65686469)

This library provides a thread-safe, cross-platform client for the TeaSpeak
and TeamSpeak 3 ServerQuery protocol.  It handles escaping, error bulks,
notification dispatching, and TeaSpeak-specific extensions, so applications
can focus on the logic they actually want to build: bots, dashboards,
moderation tools, integrations.  Commands can be issued from any thread; the
library owns a single reader thread and a single dispatcher thread.

## Required Tools

* [CMake 3.15+](https://cmake.org/)
* [Clang](https://clang.llvm.org/) or any C++17-capable compiler
  (MSVC 2019+, GCC 9+, Apple Clang 12+)
* [Visual Studio 2019+](https://www.visualstudio.com/) (on Windows)

## Sources

```bash
# With HTTPS:
git clone https://github.com/0x4D65686469/teaspeak-query.git
# With SSH:
git clone git@github.com:0x4D65686469/teaspeak-query
```

## Building on Windows

Make sure **Clang for Windows** is installed through Visual Studio Installer.

```bash
mkdir build
cd build

# Visual Studio 2019
cmake .. -G "Visual Studio 16 2019" -A x64 -T ClangCL

# Visual Studio 2022
cmake .. -G "Visual Studio 17 2022" -A x64 -T ClangCL

# Visual Studio 2026
cmake .. -G "Visual Studio 18 2026" -A x64 -T ClangCL
```

Open Visual Studio and build the solution.

Or

```bash
cmake --build . --config Debug
```

Change `Debug` to `Release` for final versions.

## Building on Linux

```bash
mkdir build
cd build
export CC=/usr/lib/llvm/13/bin/clang CXX=/usr/lib/llvm/13/bin/clang++
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

Change `Debug` to `Release` for final versions.

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

## License

This Source Code Form is subject to the terms of the **GNU General Public License, version 3 (GPLv3)**.