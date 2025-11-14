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

if(APPLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/mac/exe)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/mac/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/mac/lib)
elseif(UNIX AND NOT APPLE)
    if(USING_WSL)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/wsl/exe)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/wsl/lib)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/wsl/lib)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/linux/exe)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/linux/lib)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/linux/lib)
    endif()
elseif(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/win/exe)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/win/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/built/win/lib)
endif()


add_library(audioTool STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stdcmd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioEnv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commandQueue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioTool/management/cache.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioTool/management/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audioTool/management/resource.c
)

target_include_directories(audioTool PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/audioTool
    ${CMAKE_CURRENT_SOURCE_DIR}/structures
)

set(AUDIO_SYSTEM_SOURCES)
set(AUDSYS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/audioSystem)

if(APPLE)
    list(APPEND AUDIO_SYSTEM_SOURCES
        ${AUDSYS_DIR}/mac/audioChannel.c
        ${AUDSYS_DIR}/mac/audioManager.c
        ${AUDSYS_DIR}/mac/audioManagerTable.c
        ${AUDSYS_DIR}/mac/playWav.c
    )

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        list(APPEND AUDIO_SYSTEM_SOURCES
            ${AUDSYS_DIR}/mac/performance/arm/updateVolume.s
            ${AUDSYS_DIR}/mac/performance/arm/updateAudioDataAmplitude.s
        )
        set_source_files_properties(
            ${AUDSYS_DIR}/mac/performance/arm/updateVolume.s
            ${AUDSYS_DIR}/mac/performance/arm/updateAudioDataAmplitude.s
            PROPERTIES LANGUAGE ASM
        )
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        list(APPEND AUDIO_SYSTEM_SOURCES
            ${AUDSYS_DIR}/mac/performance/intel/updateVolume.s
            ${AUDSYS_DIR}/mac/performance/intel/updateAudioDataAmplitude.s
        )
        set_source_files_properties(
            ${AUDSYS_DIR}/mac/performance/intel/updateVolume.s
            ${AUDSYS_DIR}/mac/performance/intel/updateAudioDataAmplitude.s
            PROPERTIES LANGUAGE ASM
        )
    endif()

elseif(UNIX AND NOT APPLE)
    list(APPEND AUDIO_SYSTEM_SOURCES
        ${AUDSYS_DIR}/linux/audioChannel.c
        ${AUDSYS_DIR}/linux/audioManager.c
        ${AUDSYS_DIR}/linux/playWav.c
    )

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        list(APPEND AUDIO_SYSTEM_SOURCES
            ${AUDSYS_DIR}/linux/performance/x86/updateVolume.s
            ${AUDSYS_DIR}/linux/performance/x86/updateAudioDataAmplitude.s
        )
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        list(APPEND AUDIO_SYSTEM_SOURCES
            ${AUDSYS_DIR}/linux/performance/arm/updateVolume.s
            ${AUDSYS_DIR}/linux/performance/arm/updateAudioDataAmplitude.s
        )

    endif()

elseif(WIN32)
    list(APPEND AUDIO_SYSTEM_SOURCES
        ${AUDSYS_DIR}/win/audioChannel.c
        ${AUDSYS_DIR}/win/audioManager.c
        ${AUDSYS_DIR}/win/playWav.c
    )

    # needs add ASM sources
endif()

add_executable(totalTest ${AUDIO_SYSTEM_SOURCES} tests_public/totalTest.c)
add_executable(tutorial ${AUDIO_SYSTEM_SOURCES} tests_public/tutorial.c)

target_link_libraries(totalTest PRIVATE audioTool)
target_link_libraries(tutorial PRIVATE audioTool)

if(APPLE)
    target_link_libraries(totalTest
        PRIVATE
            "-framework AudioUnit"
    )
    target_link_libraries(tutorial
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