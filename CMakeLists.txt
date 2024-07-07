cmake_minimum_required(VERSION 3.10)
project(PATH_VIEW)

find_package(Threads REQUIRED)
find_package(llfio CONFIG REQUIRED)

add_executable(path-view-test path-view-test.cc)
target_link_libraries(path-view-test PUBLIC llfio::sl)
