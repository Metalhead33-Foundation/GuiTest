#ifndef ELVTHREADSAFECONTAINER_HPP
#define ELVTHREADSAFECONTAINER_HPP
/**
 * @file ElvThreadsafeContainer.hpp
 * @brief Declares the ElvThreadsafeContainer API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <shared_mutex>
#include <mutex>
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
	mutable std::shared_mutex mut;

public:
	/**
	 * @brief Grants non-const access to the protected data.
	 *
	 * Locks the internal mutex, executes the provided function with a reference to the data,
	 * and then unlocks the mutex upon exit.
	 *
	 * @param fun The function to execute with access to the non-const data.
	 */
	template <typename F> auto access(F&& fun) {
		std::scoped_lock lck(mut);
		return fun(data);
	}

	/**
	 * @brief Grants const access to the protected data.
	 *
	 * Locks the internal mutex, executes the provided function with a const reference to the data,
	 * and then unlocks the mutex upon exit. This method is const, ensuring the data is not modified.
	 *
	 * @param fun The function to execute with const access to the data.
	 */
	template <typename F> auto access(F&& fun) const {
		std::scoped_lock lck(mut);
		return fun(data);
	}
};
}
}
#endif // ELVTHREADSAFECONTAINER_HPP
