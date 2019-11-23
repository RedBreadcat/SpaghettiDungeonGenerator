cmake_minimum_required(VERSION 3.15)

project(SpaghettiDungeonGen)
set(CMAKE_CXX_STANDARD 17)

set(Boost_USE_MULTITHREADED ON)  

find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)
find_package(Boost REQUIRED COMPONENTS filesystem random)
include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

file(GLOB SOURCES include/*.h *.cpp)

add_executable(SpaghettiDungeonGen ${SOURCES})

target_include_directories(SpaghettiDungeonGen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(SpaghettiDungeonGen ${OpenCV_LIBS} ${Boost_LIBRARIES})