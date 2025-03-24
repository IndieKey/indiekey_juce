# IndieKey JUCE client SDK

Welcome to the IndieKey client SDK for JUCE. This SDK provides a set of classes and utilities to easily integrate
IndieKey license validation into your JUCE application or plugin .

## Getting started

There are several to integrate the SDK into your project. In all cases the SDK is provided as a JUCE module, but varies
in how dependencies are managed. We'll cover the following methods:

### Adding the distribution package to your project

We publish a distribution package that contains the SDK as a JUCE module and the dependencies as precompiled binaries.
This makes it super easy to integrate the SDK into your project, without worrying about dependencies (which can be quite
a hassle).

1. Download the latest distribution package from the [releases page](#todo).
2. Extract the contents of the package to a folder in your project.
3. When using CMake, add the module to your project using `juce_add_module(path/to/indiekey_juce)`, or
   add the module to your Projucer project if you're using Projucer.

### Adding the repository, build dependencies

Alternatively, you can add the repository as a submodule or subfolder to your project and build the dependencies using
`build.py`. The SDK uses `vcpkg` for managing dependencies and Python for the scripts. In the root of the repository
you'll find a `vcpkg.json` file that can be used to install the dependencies using `vcpkg`. There is also a `build.py`
script which does everything for you.

1. Add the repository as a submodule or subfolder to your project. Make sure to also include the vcpkg submodule (
   located at submodules/vcpkg).
2. Install the dependencies using `vcpkg`:
   ```sh
   python3 -u build.py
   ```
3. Add the `indiekey_juce` module to your project using `juce_add_module(path/to/indiekey_juce/build/indiekey_juce)`.
   Note that we're pointing to the build folder and not to the root of the repository. If you are using Projucer, you
   add the module to your Projucer project.

### Using the source code, manage dependencies yourself

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

