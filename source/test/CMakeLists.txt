# Use Catch2 v3 as a submodule
add_subdirectory(Catch2)
file(GLOB_RECURSE testfiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../**.test.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../**.test.h")
add_executable(tests ${testfiles})
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

# Load and use the .cmake file provided by Catch2
#include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
#catch_discover_tests(tests)