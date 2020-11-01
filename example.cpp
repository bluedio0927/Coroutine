#include <iostream>
#include <thread>
#include "coroutine.h"

int main()
{
	coroutine::scope_guard scope;
	while(true)
	{
		
		using inttask = coroutine::task<int>;
		using voidtask = coroutine::task<void>;

		auto ctask = inttask([](inttask::yield In_yield)
		{
			In_yield(5);
			In_yield(6);
			In_yield(7);
			return 0;
		});

		auto vtask = voidtask([](voidtask::yield In_yield)
		{
			std::cout << "A" << std::endl;
			In_yield();
			std::cout << "B" << std::endl;
			In_yield();
			std::cout << "C" << std::endl;
			In_yield();
			std::cout << "D" << std::endl;
		});

		while (ctask | vtask)
		{
			if (ctask)
				std::cout << ctask() << std::endl;

			if (vtask)
				vtask();
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}