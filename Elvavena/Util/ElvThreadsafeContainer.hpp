#ifndef ELVTHREADSAFECONTAINER_HPP
#define ELVTHREADSAFECONTAINER_HPP
#include <mutex>
#include <functional>
namespace Elv {
namespace Util {

/**
 * @brief Template class for thread-safe access to a variable of type T.
 *
 * This class provides a mechanism to access (read and/or modify) a shared variable
 * of type T in a thread-safe manner. It utilizes a recursive mutex to allow for
 * nested access calls from the same thread.
 *
 * @tparam T The type of the variable to be protected.
 */
template <typename T>
struct threadsafe {
public:
	/**
	 * @brief Type alias for a function that accesses (potentially modifies) T.
	 *
	 * This function type is used for non-const access, implying the T instance might be modified.
	 */
	typedef std::function<void(T&)> AccessorFunc;

	/**
	 * @brief Type alias for a function that accesses T without modifying it.
	 *
	 * This function type is used for const access, ensuring the T instance is not modified.
	 */
	typedef std::function<void(const T&)> ConstAccessorFunc;

	/**
	 * @brief Type alias for a lock guard on the internal recursive mutex.
	 *
	 * Automatically locks the mutex upon construction and unlocks upon destruction.
	 */
	typedef std::lock_guard<std::recursive_mutex> Lock;

private:
	/**
	 * @brief The protected data of type T.
	 */
	T data;

	/**
	 * @brief Mutable recursive mutex for thread-safe access.
	 *
	 * Declared as mutable to allow locking in const contexts (for read-only access).
	 */
	mutable std::recursive_mutex mut;

public:
	/**
	 * @brief Grants non-const access to the protected data.
	 *
	 * Locks the internal mutex, executes the provided function with a reference to the data,
	 * and then unlocks the mutex upon exit.
	 *
	 * @param fun The function to execute with access to the non-const data.
	 */
	void access(const AccessorFunc& fun) {
		Lock lck(mut);
		fun(data);
	}

	/**
	 * @brief Grants const access to the protected data.
	 *
	 * Locks the internal mutex, executes the provided function with a const reference to the data,
	 * and then unlocks the mutex upon exit. This method is const, ensuring the data is not modified.
	 *
	 * @param fun The function to execute with const access to the data.
	 */
	void access(const ConstAccessorFunc& fun) const {
		Lock lck(mut);
		fun(data);
	}
};
}
}
#endif // ELVTHREADSAFECONTAINER_HPP
