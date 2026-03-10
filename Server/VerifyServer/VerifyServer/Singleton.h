#pragma once
#include <iostream>
#include <mutex>
#include <memory>

template <typename T>
class Singleton
{
protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator=(const Singleton<T>&) = delete;

	static std::shared_ptr<T> m_instance;

public:
	~Singleton()
	{
		std::cout << "Singleton destruct" << std::endl;
	}

	void PrintAddress()
	{
		std::cout << m_instance.get() << std::endl;
	}

	static std::shared_ptr<T> GetInstance()
	{
		static std::once_flag s_flag;
		std::call_once(s_flag, [&]() {
			m_instance = std::make_shared<T>();
			});

		return m_instance;
	}
};

template <typename T>
std::shared_ptr<T> Singleton<T>::m_instance = nullptr;