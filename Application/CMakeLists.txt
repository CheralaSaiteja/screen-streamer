set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Gui)

file(GLOB SRC src/*.cpp)
file(GLOB HDR include/*.hpp)

add_executable(test-application ${SRC} ${HDR} resources.qrc)

target_link_libraries(test-application Qt6::Core Qt6::Quick Qt6::Gui)
target_include_directories(test-application PRIVATE include/)