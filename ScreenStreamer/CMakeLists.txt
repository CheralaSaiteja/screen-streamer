

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Find X11 library
find_package(X11 REQUIRED)

# Add executable target
add_executable(window-streamer-tool main.cpp)

# Link libraries
target_link_libraries(window-streamer-tool
    ${OpenCV_LIBS}
    X11
    Xfixes   # Link Xfixes directly
)

# Optional: Define any additional include directories if needed (e.g., OpenCV headers)
include_directories(${OpenCV_INCLUDE_DIRS})
