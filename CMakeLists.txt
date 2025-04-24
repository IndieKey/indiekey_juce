cmake_minimum_required(VERSION 3.24)

project(ActivationClient)

set(CMAKE_CXX_STANDARD 17)

add_library(indiekey_juce INTERFACE)

file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

target_sources(indiekey_juce INTERFACE ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(indiekey_juce INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
