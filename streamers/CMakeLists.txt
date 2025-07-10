# Find OpenCV and OpenMP
find_package(OpenCV REQUIRED)

# Add the executable
# add_executable(vstreamer-server camera-streamer-server.cpp)
add_executable(window-streamer-client camera-streamer-client.cpp)

# Link OpenCV libraries
# target_link_libraries(vstreamer-server PRIVATE ${OpenCV_LIBS})
target_link_libraries(window-streamer-client PRIVATE ${OpenCV_LIBS})

# Link V4L2 library
# find_library(V4L2_LIB v4l2 REQUIRED)
# target_link_libraries(vstreamer-server PRIVATE ${V4L2_LIB})

# Find and include V4L2 headers
# find_path(V4L2_INCLUDE_DIR NAMES linux/videodev2.h)
# if(V4L2_INCLUDE_DIR)
#     include_directories(${V4L2_INCLUDE_DIR})
# endif()

# Set C++ standard
# set_target_properties(vstreamer-server PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
set_target_properties(window-streamer-client PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)

set_target_properties(window-streamer-client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/libraries/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/libraries/Release
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/libraries/Debug
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/libraries/Release    
)

# set_target_properties(vstreamer-server PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
#     RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
#     ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/libraries/Debug
#     ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/libraries/Release
#     LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/libraries/Debug
#     LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/libraries/Release    
# )

# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     install(TARGETS vstreamer-server vstreamer-client
#         RUNTIME DESTINATION bin
#         ARCHIVE DESTINATION lib
#         LIBRARY DESTINATION lib
#     )
# else()
#     message(WARNING "Install target is only available in Release build. Current build type is: ${CMAKE_BUILD_TYPE}")
# endif()