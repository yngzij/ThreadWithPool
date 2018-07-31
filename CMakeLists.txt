cmake_minimum_required(VERSION 3.10)
project(THreadPool)

set(CMAKE_CXX_STANDARD 11)

add_executable(THreadPool main.cpp ThreadPool.cpp ThreadPool.h Locker.cpp Locker.h HttpConn.cpp HttpConn.h)