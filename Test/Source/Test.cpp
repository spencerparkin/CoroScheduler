#include "Test.h"
#include "CoroScheduler/CoroSched.h"
#include <stdio.h>

using namespace CoroScheduler;

Task CoroA();
Task CoroB();

class HappyPromise : public Promise
{
public:
	HappyPromise(int id)
	{
		this->id = id;
		this->ran = false;
	}

	virtual ~HappyPromise()
	{
	}

	virtual void Run() override
	{
		printf("Happy promise %d ran!\n", this->id);
		this->ran = true;
	}

	virtual bool IsDone() override
	{
		return this->ran;
	}

private:
	bool ran;
	int id;
};

Task CoroA()
{
	printf("Hello from the start of coroutine A!\n");

	co_yield std::make_shared<CoroPromise>(CoroB());

	printf("Hello from the end of coroutine A!\n");
}

Task CoroB()
{
	printf("Hello from the start of coroutine B!\n");

	auto promise = std::make_shared<MultiWaitPromise>(MultiWaitPromise::WAIT_ALL);

	promise->promiseArray.push_back(std::make_shared<HappyPromise>(0));
	promise->promiseArray.push_back(std::make_shared<HappyPromise>(1));
	promise->promiseArray.push_back(std::make_shared<HappyPromise>(2));

	co_yield promise;

	printf("Hello from the end of coroutine B!\n");

	co_return;
}

int main(int argc, char** argv)
{
	Scheduler scheduler;

	scheduler.AddTask(CoroA());

	while (!scheduler.IsDone())
		scheduler.Run();

	return 0;
}