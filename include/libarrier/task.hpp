#ifndef LIBARRIER_TASK_HPP
#define LIBARRIER_TASK_HPP

#include "functional.hpp"

#include <thread>

template<typename T>
class Task {
	std::thread m_thread;
	bool m_finished = false;

public:
};

#endif // LIBARRIER_TASK_HPP