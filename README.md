# IndieKey JUCE client SDK

Welcome to the IndieKey client SDK for JUCE. This SDK provides a set of classes and utilities to easily integrate
IndieKey license validation into your JUCE application or plugin.

## Getting started

There are several ways to integrate the SDK into your project. We'll cover the following methods:

### Distribution package

We publish a distribution package that contains the SDK as a JUCE module and the dependencies as precompiled binaries.
This makes it super easy to integrate the SDK into your project, without worrying about dependencies (which can be
difficult to build).

#### CMake

##### FetchContent

If you're using CMake, you can use the `FetchContent` module to download the SDK and its dependencies. This is the
recommended approach for CMake based projects.

```cmake
include(FetchContent)

FetchContent_Declare(
    indiekey_juce
    URL https://indiekey-juce.lon1.digitaloceanspaces.com/download/indiekey_juce-v0.3.0-dist.zip
)

FetchContent_MakeAvailable(indiekey_juce)
```

##### CPM

If you're using CMake and prefer to use [CPM](https://github.com/cpm-cmake/CPM.cmake) to manage your dependencies, you can use the following code snippet to download the SDK and its dependencies.

```cmake
include(cmake/CPM.cmake)

CPMAddPackage(
        NAME indiekey_juce
        URL https://indiekey-juce.lon1.digitaloceanspaces.com/download/indiekey_juce-v0.3.0-dist.zip
)
``` 

##### Linking

Once the library is added through `FetchContent` or `CPM`, you can link it to your target using the `target_link_libraries` command:

```cmake
target_link_libraries(YourTarget
        PRIVATE
        juce::juce_core
        indiekey_juce
        
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

#### Projucer

If you're using Projucer, you can add the module to your project by following these steps:

1. Download the latest distribution package from the [releases page](#todo).
2. Extract the contents of the package to a folder in your project.
3. Open your Projucer project.
2. Go to the "Modules" tab.
3. Click on the "Add a module" button.
4. Select the "Add a module from a folder" option.
5. Navigate to the folder where you extracted the SDK and select the `indiekey_juce` folder.
6. Click "Open" to add the module to your project.

### Adding the repository, manually build dependencies

Alternatively, you can add the repository as a submodule or subfolder to your project and build the dependencies using
`build.py`. The SDK uses `vcpkg` for managing dependencies and Python for the scripts. In the root of the repository
you'll find a `vcpkg.json` file that can be used to install the dependencies using `vcpkg`. 

1. Add the repository as a submodule or subfolder to your project. Make sure to also include the vcpkg submodule (
   located at submodules/vcpkg).
2. Install the dependencies using `vcpkg`:
   ```sh
   python3 -u build.py
   ```
3. Add the `indiekey_juce` module to your project using `juce_add_module(path/to/indiekey_juce/build/indiekey_juce)`.
   Note that we're pointing to the build folder and not to the root of the repository. If you are using Projucer, you
   add the module to your Projucer project.

### Using the source code directly, manually manage dependencies

Lastly, if you prefer to have full control over the dependencies, you can add the repository as a submodule or subfolder
and build the SDK from source. This requires you to manage the dependencies yourself. The recommended approach is to use
`vcpkg`.

The SDK requires the following dependencies:

- [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) (MIT License)
- [libsodium](https://doc.libsodium.org/) (ISC License)
- [sqlite3](https://www.sqlite.org/index.html)
- [nlohmann-json](https://github.com/nlohmann/json) (MIT License)

JUCE:

- juce_core

