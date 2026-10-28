# Run the real decoder against independently generated waveforms using Saleae's SDK mocks.
set(harness_files AnalyzerStubs.cpp HelperStubs.cpp MockChannelData.cpp
    MockSimulatedChannelDescriptor.cpp MockResults.cpp SettingsStubs.cpp
    StreamHelpers.cpp TestInstance.cpp)
set(harness_sources)
foreach(file IN LISTS harness_files)
    list(APPEND harness_sources "${analyzersdk_SOURCE_DIR}/testlib/${file}")
endforeach()
add_library(analyzer_test_harness STATIC ${harness_sources})
target_include_directories(analyzer_test_harness PUBLIC
    "${analyzersdk_SOURCE_DIR}/include" "${analyzersdk_SOURCE_DIR}/testlib")
# The upstream test harness relies on transitive standard-library includes.
target_compile_options(analyzer_test_harness PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/TestPrelude.h")
foreach(rate IN LISTS DMX_RATES_KBAUD)
    math(EXPR baud "${rate} * 1000")
    set(target test_dmx_${rate})
    add_executable(${target} DecodeTests.cpp ${PROJECT_SOURCE_DIR}/src/Dmx512Analyzer.cpp
        ${PROJECT_SOURCE_DIR}/src/Dmx512AnalyzerResults.cpp
        ${PROJECT_SOURCE_DIR}/src/Dmx512AnalyzerSettings.cpp
        ${PROJECT_SOURCE_DIR}/src/Dmx512SimulationDataGenerator.cpp)
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_compile_options(${target} PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/TestPrelude.h")
    target_compile_definitions(${target} PRIVATE LOGIC2 DMX_BIT_RATE=${baud}
        "DMX_ANALYZER_NAME=\"DMX-512 (${rate} kbaud)\"")
    target_link_libraries(${target} PRIVATE analyzer_test_harness)
    add_test(NAME decode_${rate}_kbaud COMMAND ${target})
    set_tests_properties(decode_${rate}_kbaud PROPERTIES TIMEOUT 30)
endforeach()
