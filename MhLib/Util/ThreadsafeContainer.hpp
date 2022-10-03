#ifndef THREADSAFECONTAINER_HPP
#define THREADSAFECONTAINER_HPP
#include <mutex>
#include <functional>

template <typename T> struct threadsafe {
public:
	typedef std::function<void(T&)> AccessorFunc;
	typedef std::function<void(const T&)> ConstAccessorFunc;
	typedef std::lock_guard<std::recursive_mutex> Lock;
private:
	T data;
	mutable std::recursive_mutex mut;
public:
	void access(const AccessorFunc& fun) {
		Lock lck(mut);
		fun(data);
	}
	void access(const ConstAccessorFunc& fun) const {
		Lock lck(mut);
		fun(data);
	}
};

#endif // THREADSAFECONTAINER_HPP
