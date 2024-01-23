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
    libminiupnpc.dylib
    HINTS "miniupnp/miniupnpc" "miniupnp/miniupnpc/build"
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

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	${MINIUPNPC}
)

include(~/Documents/Max\ 8/Packages/max-sdk/source/max-sdk-base/script/max-posttarget.cmake)
