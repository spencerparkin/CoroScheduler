# CoroScheduler library

set(CORO_SCHED_SOURCES
	Source/CoroScheduler/CoroSched.cpp
	Source/CoroScheduler/CoroSched.h
)

add_library(CoroScheduler STATIC ${CORO_SCHED_SOURCES})

target_include_directories(CoroScheduler PUBLIC Source)