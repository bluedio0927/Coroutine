#pragma once

#include <functional>
#include <cassert>
#include <type_traits>

#include <Windows.h>

namespace coroutine
{
	inline static void *& main()
	{
		static thread_local void* main = nullptr;
		return main;
	}

	struct scope_guard
	{
		scope_guard()
		{
			main() = ConvertThreadToFiber(NULL);
		}

		~scope_guard()
		{
			ConvertFiberToThread();
			main() = nullptr;
		}
	};

	template<typename T>
	class task
	{
	public:
		using yield = std::function<void(T)>;

		using func = std::function<T(yield)>;

	public:

		task(func &&InFunc, size_t In_StackSize = 128 * 1024)
			:m_Func(InFunc), m_bNext(false), m_Val(nullptr)
		{
			constexpr size_t commit_size = 16 * 1024;
			m_Fiber = CreateFiberEx(commit_size, max(commit_size, In_StackSize), FIBER_FLAG_FLOAT_SWITCH, Run, this);
			
			if (m_Fiber)
				m_bNext = true;
			else
				throw std::exception("CreateFiberEx Failed");
		}

		~task()
		{
			assert(m_bNext == false);
			if (m_Fiber)
				DeleteFiber(m_Fiber);
		}

		template<typename V = T>
		inline typename std::enable_if<std::is_void<V>::value, V>::type operator()()
		{
			SwitchToFiber(m_Fiber);
		}

		template<typename V = T>
		inline typename std::enable_if<!std::is_void<V>::value, V>::type operator()()
		{
			SwitchToFiber(m_Fiber);
			return *m_Val;
		}

		inline operator bool() const noexcept
		{
			return m_bNext;
		}

	private:

		template<typename V = T>
		typename std::enable_if<!std::is_void<V>::value, void>::type yield_func(V In_val) noexcept
		{
			*m_Val = In_val;
			SwitchToFiber(main());
		}

		template<typename V = T>
		typename std::enable_if<std::is_void<V>::value, void>::type yield_func() noexcept
		{
			SwitchToFiber(main());
		}

		template<typename V = T>
		static typename std::enable_if<std::is_void<V>::value, void>::type __stdcall Run(void *In_pContext)
		{
			auto pTask = (task *)In_pContext;
			pTask->m_Func(std::bind(&task::yield_func<V>, pTask));
			pTask->m_bNext = false;
			SwitchToFiber(main());
		}

		template<typename V = T>
		static typename std::enable_if<!std::is_void<V>::value, void>::type __stdcall Run(void *In_pContext)
		{
			auto pTask = (task *)In_pContext;
			pTask->m_Val = new T();
			*(pTask->m_Val) = pTask->m_Func(std::bind(&task::yield_func<V>, pTask, std::placeholders::_1));
			delete pTask->m_Val;

			pTask->m_bNext = false;
			SwitchToFiber(main());
		}

	private:
		func m_Func;
		bool m_bNext;
		void *m_Fiber;
		T* m_Val;
	};
}
