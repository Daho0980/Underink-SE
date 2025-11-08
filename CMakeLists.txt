cmake_minimum_required(VERSION 3.24)
project(UnderinkSE LANGUAGES C ASM)

if(DEFINED ENV{WSL_DISTRO_NAME})
    set(USING_WSL 1)
else()
    set(USING_WSL 0)
endif()

message(STATUS "######################")
message(STATUS "Arch: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "######################")

add_library(audioTool SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioEnv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commandQueue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioTool/management/cache.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioTool/management/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioTool/management/resource.c
)

target_include_directories(audioTool
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/audioTool
        ${CMAKE_CURRENT_SOURCE_DIR}/structures
)

if(APPLE)
    set_target_properties(audioTool PROPERTIES
        INSTALL_NAME_DIR "@rpath"
        BUILD_RPATH      "@loader_path"
        INSTALL_RPATH    "@loader_path"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(audioTool PROPERTIES
        BUILD_RPATH   "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

set(AUDIO_THREAD_SOURCES tests/AMgrTableTest.c)

if(APPLE)
    list(APPEND AUDIO_THREAD_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/audioThread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/playWav.c
    )

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        list(APPEND AUDIO_THREAD_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/arm/updateVolume.s
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/arm/updateAudioDataAmplitude.s
        )
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/arm/updateVolume.s
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/arm/updateAudioDataAmplitude.s
            PROPERTIES LANGUAGE ASM
        )
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        list(APPEND AUDIO_THREAD_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/intel/updateVolume.s
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/intel/updateAudioDataAmplitude.s
        )
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/intel/updateVolume.s
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/mac/performance/intel/updateAudioDataAmplitude.s
            PROPERTIES LANGUAGE ASM
        )
    endif()

elseif(UNIX AND NOT APPLE)
    list(APPEND AUDIO_THREAD_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/linux/audioThread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/linux/playWav.c
    )

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        list(APPEND AUDIO_THREAD_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/linux/performance/x86/updateVolume.s
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/linux/performance/x86/updateAudioDataAmplitude.s
        )
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        list(APPEND AUDIO_THREAD_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/linux/performance/arm/updateVolume.s
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/linux/performance/arm/updateAudioDataAmplitude.s
        )

    endif()

elseif(WIN32)
    list(APPEND AUDIO_THREAD_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/win/audioThread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audioThread/win/playWav.c
    )

    # needs add ASM sources
endif()

add_executable(UnderinkSE ${AUDIO_THREAD_SOURCES})

if(APPLE)
    set_target_properties(UnderinkSE PROPERTIES
        BUILD_PATH    "@executable_path"
        INSTALL_RPATH "@executable_path"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(UnderinkSE PROPERTIES
        BUILD_PATH    "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

target_link_libraries(UnderinkSE PRIVATE audioTool)

if(APPLE)
    target_link_libraries(UnderinkSE
        PRIVATE
            "-framework AudioUnit"
    )
elseif(UNIX AND NOT APPLE)
    if(USING_WSL)
    # for WSL. 
    endif()
# SOON!
elseif(WIN32)
# SOON!
endif()