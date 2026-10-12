# CoroScheduler test

set(CORO_SCHED_TEST_SOURCES
	Source/Test.cpp
	Source/Test.h
)

add_executable(CoroSchedulerTest ${CORO_SCHED_TEST_SOURCES})

target_link_libraries(CoroSchedulerTest PRIVATE
	CoroScheduler
)