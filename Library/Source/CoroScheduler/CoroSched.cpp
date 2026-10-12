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

bool Task::IsDone()
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

std::suspend_always Task::promise_type::yield_value(std::shared_ptr<Promise> promise)
{
	this->promise = promise;
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

bool CoroPromise::IsDone()
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
		if (!promise->IsDone())
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
		if (!promise->IsDone())
			promise->Run();
	}
}

/*virtual*/ bool MultiWaitPromise::IsDone()
{
	int doneCount = 0;

	for (int i = 0; i < (int)this->promiseArray.size(); i++)
	{
		Promise* promise = this->promiseArray[i].get();
		if (promise->IsDone())
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

void Scheduler::AddPromise(std::shared_ptr<Promise> promise)
{
	this->promiseArray.push_back(promise);
}

void Scheduler::AddTask(Task&& task)
{
	auto promise = std::make_shared<CoroPromise>(std::move(task));
	this->AddPromise(promise);
}

void Scheduler::Run()
{
	this->tickCount++;

	int j = -1;
	int largestDelta = 0;

	// A priority queue would be more efficient here, but this is fine for now.
	for (int i = 0; i < (int)this->promiseArray.size(); i++)
	{
		Promise* promise = this->promiseArray[i].get();
		int delta = this->tickCount - promise->lastRunTick;
		if (delta > largestDelta)
		{
			largestDelta = delta;
			j = i;
		}
	}

	if (j == -1)
		return;

	Promise* promise = this->promiseArray[j].get();
	promise->lastRunTick = this->tickCount;
	promise->Run();

	if (promise->IsDone())
	{
		this->promiseArray[j] = nullptr;

		int k = (int)this->promiseArray.size() - 1;
		if (j != k)
			this->promiseArray[j] = this->promiseArray[k];

		this->promiseArray.pop_back();
	}
}

bool Scheduler::IsDone()
{
	return this->promiseArray.size() == 0;
}