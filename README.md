# libmrim

Client (library) implementation of MRIM (Mail.Ru Agent Instant Messaing) protocol using Qt.


(WIP)


## Dependencies

- Qt6 (Core, Network, Core5Compat)
- C++17-able compiler (e.g. anything these days)


## Building

```bash
git clone https://github.com/relativemodder/libmrim
cd libmrim && mkdir build && cd build
cmake .. && cmake --build .
```

## How to use `libmrim` in other CMake projects

```cmake
add_subdirectory(/path/to/libmrim libmrim)

...

target_include_directories(testclient PUBLIC /path/to/libmrim)
```