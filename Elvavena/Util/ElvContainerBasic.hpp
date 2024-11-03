#ifndef ELVCONTAINERBASIC_HPP
#define ELVCONTAINERBASIC_HPP
#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>
namespace Elv {
namespace Util {

template<typename T>
concept ContainerCommon = requires(T container, const T const_container) {
	typename T::value_type;
	typename T::reference;
	typename T::const_reference;
	typename T::iterator;
	typename T::const_iterator;
	typename T::difference_type;
	typename T::size_type;

	// Common operations
	{ container.begin() } -> std::same_as<typename T::iterator>;
	{ container.end() } -> std::same_as<typename T::iterator>;
	{ const_container.begin() } -> std::same_as<typename T::const_iterator>;
	{ const_container.end() } -> std::same_as<typename T::const_iterator>;
	{ container.empty() } -> std::same_as<bool>;
	{ container.size() } -> std::same_as<typename T::size_type>;
	{ container.max_size() } -> std::same_as<typename T::size_type>;
	{ container.clear() } -> std::same_as<void>;
};

template<typename T>
concept VectorLike = ContainerCommon<T> && requires(T container,
	const typename T::value_type& value,
	typename T::size_type pos,
	typename T::size_type count) {

	// Vector-specific operations
	{ container.push_back(value) } -> std::same_as<void>;
	{ container.pop_back() } -> std::same_as<void>;
	{ container[pos] } -> std::same_as<typename T::reference>;
	{ container.data() } -> std::same_as<typename T::value_type*>;
	{ container.reserve(count) } -> std::same_as<void>;
	{ container.resize(count) } -> std::same_as<void>;
	{ container.capacity() } -> std::same_as<typename T::size_type>;
	// Random access requirement
	requires std::random_access_iterator<typename T::iterator>;
};

template<typename T>
concept MapLike = ContainerCommon<T> && requires(T container,
	const typename T::key_type& key,
	const typename T::mapped_type& value) {

	// Map-specific types
	typename T::key_type;
	typename T::mapped_type;
	requires std::same_as<typename T::value_type, std::pair<const typename T::key_type, typename T::mapped_type>>;

	// Map-specific operations
	{ container[key] } -> std::same_as<typename T::mapped_type&>;
	{ container.find(key) } -> std::same_as<typename T::iterator>;
	{ container.count(key) } -> std::same_as<typename T::size_type>;
	{ container.contains(key) } -> std::same_as<bool>;
	{ container.insert({key, value}) } -> std::same_as<std::pair<typename T::iterator, bool>>;
	{ container.erase(key) } -> std::same_as<typename T::size_type>;

	// Ensure iterator's value_type matches the container's value_type
	requires std::same_as<typename std::iterator_traits<typename T::iterator>::value_type, typename T::value_type>;
};


template<typename T>
concept SequentialContainer = ContainerCommon<T> &&
	!VectorLike<T> &&
	!MapLike<T> &&
	requires(T container, const typename T::value_type& value) {

	// Sequential container specific operations
	{ container.insert(container.begin(), value) } -> std::same_as<typename T::iterator>;
	{ container.erase(container.begin()) } -> std::same_as<typename T::iterator>;
};

}
}

#endif // ELVCONTAINERBASIC_HPP
