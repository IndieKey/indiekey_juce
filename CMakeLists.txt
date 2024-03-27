project(ActivationClient)

set(CMAKE_CXX_STANDARD 17)

add_library(indiekey_juce INTERFACE)

target_sources(indiekey_juce
        INTERFACE
        # Headers
        include/indiekey/ActivationClient.h
        include/indiekey/RestClient.h
        include/indiekey/Activation.h
        include/indiekey/Crypto.h
        include/indiekey/ActivationsDatabase.h
        include/indiekey/util/ScopedFunction.h

        # Sources
        src/ActivationClient.cpp
        src/RestClient.cpp
        src/Activation.cpp
        src/Crypto.cpp
        src/ActivationsDatabase.cpp
)

target_include_directories(indiekey_juce
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libsodium/include
)
