set(C74_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../externals")
set(CMAKE_OSX_ARCHITECTURES x86_64;arm64)

string(REGEX REPLACE "(.*)/" "" THIS_FOLDER_NAME "${CMAKE_CURRENT_SOURCE_DIR}")
project(${THIS_FOLDER_NAME})

include(~/Documents/Max\ 8/Packages/max-sdk/source/max-sdk-base/script/max-pretarget.cmake)

#############################################################
# MAX EXTERNAL
#############################################################

include_directories( 
	"${MAX_SDK_INCLUDES}"
	"${MAX_SDK_MSP_INCLUDES}"
	"${MAX_SDK_JIT_INCLUDES}"
	"miniupnp/miniupnpc"
	"miniupnp/miniupnpc/build"
	"miniupnp/miniupnpc/include"
)

find_library (
   MINIUPNPC
   if (APPLE)
   	libminiupnpc.dylib
   else if(WIN32)
		miniupnpc
	endif ()
   HINTS "miniupnp/miniupnpc" "miniupnp/miniupnpc/build" "miniupnp/miniupnpc/build/Debug" "miniupnp/miniupnpc/build/Release"
   NO_DEFAULT_PATH
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

if (WIN32)
	if(MSVC)
		#find_library(WS2_32 ws2_32.lib)
		#cmake_print_variables(WS2_32)
		target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32.lib iphlpapi.lib)
	else()
		target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32 iphlpapi)
	endif()
endif ()

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	${MINIUPNPC}
)

include(~/Documents/Max\ 8/Packages/max-sdk/source/max-sdk-base/script/max-posttarget.cmake)
