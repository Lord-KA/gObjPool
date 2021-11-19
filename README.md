# gObjPool
This is a default C allocator wrapper for better memory management in node-based data structures

## Building
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Using in your project
Add code below to your CMakeLists.txt and include "gobjpool.h"
```
FetchContent_Declare(
  objpool
  GIT_REPOSITORY https://github.com/Lord-KA/gObjPool.git
  GIT_TAG        release-1.X
)
if(NOT gobjpool_POPULATED)
  FetchContent_Populate(gobjpool)
  include_directories(${gobjpool_SOURCE_DIR})
endif()
```
You have to pre-define `GOBJPOOL_TYPE` with macro or `typedef` before including the header

## DONE
1. Generalized objPool structure with automatic refitting (reallocation)
2. Basic unit tests
3. CMake config
4. Github release

## TODO
1. Improve unit testing
2. Check coverage
3. Add Github CI
4. Add C-style pseudo-templates
5. Add docs
