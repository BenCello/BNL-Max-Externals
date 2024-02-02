cmake_minimum_required(VERSION 3.19)

string(REGEX REPLACE "(.*)/" "" THIS_FOLDER_NAME "${CMAKE_CURRENT_SOURCE_DIR}")
project(${THIS_FOLDER_NAME})

#############
# MINIUPNPC #
#############

if (APPLE)
	set(CMAKE_OSX_ARCHITECTURES x86_64;arm64)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../externals/bc.upnpc.mxo/Contents/Resources")
	set(MINIUPNPC ${C74_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.mxo/Contents/Resources/libminiupnpc.dylib)
elseif (WIN32)
	set(MINIUPNPC ${CMAKE_CURRENT_BINARY_DIR}/miniupnp/miniupnpc/$<CONFIG>/miniupnpc.lib)
endif ()

add_subdirectory("miniupnp/miniupnpc")

#############################################################
# MAX EXTERNAL
#############################################################
set(C74_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../externals")
include(../max-sdk-base/script/max-pretarget.cmake)

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

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	${MINIUPNPC}
)

if (WIN32)
	if(MSVC)
		target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32.lib iphlpapi.lib)
	else()
		target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32 iphlpapi)
	endif()
elseif (APPLE)
 	add_custom_command(TARGET ${PROJECT_NAME}
      POST_BUILD COMMAND 
      ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@loader_path/../Resources/"
      $<TARGET_FILE:${PROJECT_NAME}>)
endif ()

include(../max-sdk-base/script/max-posttarget.cmake)
