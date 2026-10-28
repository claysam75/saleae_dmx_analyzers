cmake_minimum_required(VERSION 3.15)
project(saleae_dmx_analyzers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum macOS version")

include(FetchContent)
FetchContent_Declare(analyzersdk
    GIT_REPOSITORY https://github.com/saleae/AnalyzerSDK.git
    GIT_TAG e3c4b4ffda1c162c024a5913520aed17c6edb238
)
FetchContent_MakeAvailable(analyzersdk)
include("${analyzersdk_SOURCE_DIR}/AnalyzerSDKConfig.cmake")

set(ANALYZER_SOURCES
    src/Dmx512Analyzer.cpp
    src/Dmx512AnalyzerResults.cpp
    src/Dmx512AnalyzerSettings.cpp
    src/Dmx512SimulationDataGenerator.cpp
)
set(DMX_RATES_KBAUD 250 500 750 1000 1250 1500)
foreach(rate IN LISTS DMX_RATES_KBAUD)
    math(EXPR baud "${rate} * 1000")
    set(target dmx_512_analyzer_${rate})
    add_library(${target} MODULE ${ANALYZER_SOURCES})
    target_compile_definitions(${target} PRIVATE LOGIC2
        DMX_BIT_RATE=${baud}
        "DMX_ANALYZER_NAME=\"DMX-512 (${rate} kbaud)\"")
    target_link_libraries(${target} PRIVATE Saleae::AnalyzerSDK)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Analyzers"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Analyzers")
    install(TARGETS ${target} RUNTIME DESTINATION Analyzers LIBRARY DESTINATION Analyzers)
endforeach()

include(CTest)
if(BUILD_TESTING AND UNIX AND NOT APPLE)
    add_subdirectory(tests)
endif()

