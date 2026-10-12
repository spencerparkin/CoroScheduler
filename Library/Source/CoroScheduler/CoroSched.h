#pragma once

#include <coroutine>
#include <memory>
#include <vector>

namespace CoroScheduler
{
	class Promise;

	/**
	 * Coroutines should return one of these.
	 */
	class Task final
	{
	public:
		class promise_type;
		using CoroHandle = std::coroutine_handle<promise_type>;

		Task() = delete;
		Task(const Task& task) = delete;
		explicit Task(promise_type* pt);
		Task(Task&& task) noexcept;
		~Task();

		void operator=(const Task& task) = delete;
		void operator=(Task&& task) noexcept;

		void Run();
		bool IsDone();

		class promise_type
		{
		public:
			Task get_return_object();

			std::suspend_always initial_suspend() noexcept;
			std::suspend_always final_suspend() noexcept;
			std::suspend_always yield_value(std::shared_ptr<Promise> promise);

			void return_void() noexcept;
			void unhandled_exception() noexcept;

			std::shared_ptr<Promise> promise;
		};

	private:
		CoroHandle coroHandle;
	};

	/**
	 * This is the base class for anything that runs and potentially produces
	 * a result in the task schedualing system.  This could be a coroutine or
	 * a network request or anything, really.  The result, if any, is meant to
	 * be something owned by a derivative of this class.  A shared pointer to
	 * the promise can be stored in the coroutine that yields, or the regular
	 * routine that added it to the scheduler.
	 */
	class Promise
	{
		friend class Scheduler;

	public:
		Promise();
		virtual ~Promise();

		virtual void Run() = 0;
		virtual bool IsDone() = 0;

	private:
		int lastRunTick;
	};

	/**
	 * This promise simply resumes a coroutine unless that coroutine is waiting
	 * for some other promise.
	 */
	class CoroPromise : public Promise
	{
	public:
		CoroPromise(const Task& task) = delete;
		CoroPromise(Task&& task);
		virtual ~CoroPromise();

		virtual void Run() override;
		virtual bool IsDone() override;

	protected:
		Task task;
	};

	/**
	 * This promise lets you wait until all or any of the stores promises are done.
	 */
	class MultiWaitPromise : public Promise
	{
	public:
		enum Mode
		{
			WAIT_ALL,
			WAIT_ANY
		};

		MultiWaitPromise(Mode mode);
		MultiWaitPromise(Mode mode, std::vector<std::shared_ptr<Promise>> promiseArray);
		virtual ~MultiWaitPromise();

		virtual void Run() override;
		virtual bool IsDone() override;

	public:
		std::vector<std::shared_ptr<Promise>> promiseArray;
		Mode mode;
	};

	/**
	 * This is where we pump all the promises.  The user should make an instance of this
	 * and then call its @ref Run method once per iteration loop of their program.
	 */
	class Scheduler
	{
	public:
		Scheduler();
		virtual ~Scheduler();

		void AddPromise(std::shared_ptr<Promise> promise);
		void AddTask(Task&& task);
		void Run();
		bool IsDone();

	protected:

		std::vector<std::shared_ptr<Promise>> promiseArray;
		int tickCount;
	};
}