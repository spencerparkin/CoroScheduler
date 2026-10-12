# CoroSchedular

cmake_minimum_required(VERSION 3.26)

project(CoroScheduler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CORO_SCHED_TEST)
	set(CORO_SCHED_TEST ON)
endif()

add_subdirectory(Library)

if(CORO_SCHED_TEST)
	add_subdirectory(Test)
endif()