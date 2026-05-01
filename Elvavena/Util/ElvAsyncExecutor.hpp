#ifndef ELVASYNCEXECUTOR_HPP
#define ELVASYNCEXECUTOR_HPP
#include <concepts>
#include <functional>
namespace Elv {
namespace Util {

template<typename T> concept AsyncExecutor =
	requires(T& executor, std::function<void()> task)
{
	{ executor.enqueue(std::move(task)) } -> std::same_as<void>;
	{ executor.enqueueAsync([] {}) };
	{ executor.enqueueSync([] {}) };
};

}
}
#endif // ELVASYNCEXECUTOR_HPP
