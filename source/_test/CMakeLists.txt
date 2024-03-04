# Use Catch2 v3 as a submodule
add_subdirectory(Catch2)

string(REGEX REPLACE "(.*)/" "" THIS_FOLDER_NAME "${CMAKE_CURRENT_SOURCE_DIR}")
project(${THIS_FOLDER_NAME})

#include(../max-sdk-base/script/max-pretarget.cmake)

#include_directories( 
#  "${MAX_SDK_INCLUDES}"
#  "${MAX_SDK_MSP_INCLUDES}"
#  "${MAX_SDK_JIT_INCLUDES}"
#)

# glob all *.test.cpp files from externals folders
file(GLOB_RECURSE testfiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../**.test.cpp")
file(COPY "midi_freqs.txt" DESTINATION .)

add_executable(${PROJECT_NAME} ${testfiles})
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

#if (APPLE)
#  #set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-F'${CMAKE_CURRENT_SOURCE_DIR}/../max-sdk-base/c74support/max-includes', -weak_framework MaxAPI")
#  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-F'${CMAKE_CURRENT_SOURCE_DIR}/../max-sdk-base/c74support/msp-includes', -weak_framework MaxAudioAPI")
#  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-F'${CMAKE_CURRENT_SOURCE_DIR}/../max-sdk-base/c74support/jit-includes', -weak_framework JitterAPI")
#  #target_compile_options(${PROJECT_NAME} PRIVATE -DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS)
#endif ()

# Load and use the .cmake file provided by Catch2
#include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
#catch_discover_tests(${PROJECT_NAME})

#include(../max-sdk-base/script/max-posttarget.cmake)