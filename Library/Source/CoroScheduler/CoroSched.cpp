#include "CoroScheduler/CoroSched.h"

using namespace CoroScheduler;

//----------------------------------- Task -----------------------------------

Task::Task(promise_type* pt) : coroHandle{ CoroHandle::from_promise(*pt) }
{
}

Task::Task(Task&& task) noexcept : coroHandle{ std::exchange(task.coroHandle, nullptr) }
{
}

Task::~Task()
{
	if (this->coroHandle)
		this->coroHandle.destroy();
}

void Task::operator=(Task&& task) noexcept
{
	this->coroHandle = std::exchange(task.coroHandle, nullptr);
}

void Task::Run()
{
	Promise* promise = this->coroHandle.promise().promise.get();
	if (promise)
	{
		promise->Run();

		if (!promise->IsDone())
			return;

		this->coroHandle.promise().promise = nullptr;
	}

	if (!this->coroHandle.done())
		this->coroHandle.resume();
}

bool Task::IsDone() const
{
	Promise* promise = this->coroHandle.promise().promise.get();
	if (promise && !promise->IsDone())
		return false;

	return this->coroHandle.done();
}

//----------------------------------- Task::promise_type -----------------------------------

Task Task::promise_type::get_return_object()
{
	return Task(this);
}

std::suspend_always Task::promise_type::initial_suspend() noexcept
{
	return {};
}

std::suspend_always Task::promise_type::final_suspend() noexcept
{
	return {};
}

std::suspend_always Task::promise_type::yield_value(Task&& task)
{
	this->promise = std::make_shared<CoroPromise>(std::move(task));
	return {};
}

std::suspend_always Task::promise_type::yield_value(std::shared_ptr<Promise> yielded_promise)
{
	this->promise = yielded_promise;
	return {};
}

void Task::promise_type::return_void() noexcept
{
}

void Task::promise_type::unhandled_exception() noexcept
{
}

//----------------------------------- Promise -----------------------------------

Promise::Promise()
{
	this->lastRunTick = -1;
}

/*virtual*/ Promise::~Promise()
{
}

//----------------------------------- CoroPromise -----------------------------------

CoroPromise::CoroPromise(Task&& task) : task(std::move(task))
{
}

/*virtual*/ CoroPromise::~CoroPromise()
{
}

/*virtual*/ void CoroPromise::Run()
{
	this->task.Run();
}

bool CoroPromise::IsDone() const
{
	return this->task.IsDone();
}

//----------------------------------- MultiWaitPromise -----------------------------------

MultiWaitPromise::MultiWaitPromise(Mode mode)
{
	this->mode = mode;
}

MultiWaitPromise::MultiWaitPromise(Mode mode, std::vector<std::shared_ptr<Promise>> promiseArray)
{
	this->mode = mode;

	for (int i = 0; i < (int)this->promiseArray.size(); i++)
	{
		std::shared_ptr<Promise> promise = this->promiseArray[i];
		if (promise.get() && !promise->IsDone())
			this->promiseArray.push_back(promise);
	}
}

/*virtual*/ MultiWaitPromise::~MultiWaitPromise()
{
}

/*virtual*/ void MultiWaitPromise::Run()
{
	for (int i = 0; i < (int)this->promiseArray.size(); i++)
	{
		Promise* promise = this->promiseArray[i].get();
		if (promise && !promise->IsDone())
			promise->Run();
	}
}

/*virtual*/ bool MultiWaitPromise::IsDone() const
{
	int doneCount = 0;

	for (int i = 0; i < (int)this->promiseArray.size(); i++)
	{
		Promise* promise = this->promiseArray[i].get();
		if (!promise || promise->IsDone())
			doneCount++;
	}

	if (this->mode == Mode::WAIT_ALL)
		return doneCount == (int)this->promiseArray.size();

	if (this->mode == Mode::WAIT_ANY)
		return doneCount > 0;

	return false;
}

//----------------------------------- Scheduler -----------------------------------

Scheduler::Scheduler()
{
	this->tickCount = 0;
}

/*virtual*/ Scheduler::~Scheduler()
{
}

void Scheduler::AddPromise(const std::string& promiseName, std::shared_ptr<Promise> promise)
{
	this->promiseMap.insert(std::pair(promiseName, promise));
}

void Scheduler::AddTask(const std::string& promiseName, Task&& task)
{
	auto promise = std::make_shared<CoroPromise>(std::move(task));
	this->AddPromise(promiseName, promise);
}

bool Scheduler::IsPromiseStillPending(const std::string& promiseName)
{
	return this->promiseMap.find(promiseName) != this->promiseMap.end();
}

/*virtual*/ void Scheduler::Run()
{
	this->tickCount++;

	std::string oldestPromiseName;
	int largestDelta = 0;
	Promise* oldestPromise = nullptr;

	// A priority queue would be more efficient here, but this is fine for now.
	for (auto pair : this->promiseMap)
	{
		Promise* promise = pair.second.get();
		int delta = this->tickCount - promise->lastRunTick;
		if (delta > largestDelta)
		{
			largestDelta = delta;
			oldestPromise = promise;
			oldestPromiseName = pair.first;
		}
	}

	if (!oldestPromise)
		return;

	oldestPromise->lastRunTick = this->tickCount;
	oldestPromise->Run();

	if (oldestPromise->IsDone())
		this->promiseMap.erase(oldestPromiseName);
}

/*virtual*/ bool Scheduler::IsDone() const
{
	return this->promiseMap.size() == 0;
}