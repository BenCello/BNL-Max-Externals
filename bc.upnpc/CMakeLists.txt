set(C74_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../externals")
string(REGEX REPLACE "(.*)/" "" THIS_FOLDER_NAME "${CMAKE_CURRENT_SOURCE_DIR}")

project(${THIS_FOLDER_NAME})
include(~/Documents/Max\ 8/Packages/max-sdk/source/max-sdk-base/script/max-pretarget.cmake)

#############
# MINIUPNPC #
#############

set(CMAKE_OSX_ARCHITECTURES x86_64;arm64)
add_subdirectory("miniupnp/miniupnpc")

#############################################################
# MAX EXTERNAL
#############################################################
set(CMAKE_OSX_ARCHITECTURES x86_64;arm64)

include_directories( 
	"${MAX_SDK_INCLUDES}"
	"${MAX_SDK_MSP_INCLUDES}"
	"${MAX_SDK_JIT_INCLUDES}"
	"miniupnp/miniupnpc"
	"miniupnp/miniupnpc/build"
	"miniupnp/miniupnpc/include"
)

file(GLOB PROJECT_SRC
     "*.h"
	 "*.c"
     "*.cpp"
     "miniupnp/miniupnpc/include/*.h"
     "miniupnp/miniupnpc/src/listdevices.c"
)

add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_SRC}
)

add_dependencies(${PROJECT_NAME} libminiupnpc-shared)

if (APPLE)
	set(MINIUPNPC ${C74_LIBRARY_OUTPUT_DIRECTORY}/libminiupnpc.dylib)
elseif(WIN32)
	set(MINIUPNPC ${C74_LIBRARY_OUTPUT_DIRECTORY}/miniupnpc)
endif ()

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	${MINIUPNPC}
)

if (WIN32)
	if(MSVC)
		#find_library(WS2_32 ws2_32.lib)
		#cmake_print_variables(WS2_32)
		target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32.lib iphlpapi.lib)
	else()
		target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32 iphlpapi)
	endif()
endif ()

include(~/Documents/Max\ 8/Packages/max-sdk/source/max-sdk-base/script/max-posttarget.cmake)
