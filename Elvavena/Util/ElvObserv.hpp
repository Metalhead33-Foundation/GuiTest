#ifndef ELVOBSERV_HPP
#define ELVOBSERV_HPP
#include <memory>
#include <exception>
namespace Elv {
namespace Util {

/**
 * @defgroup ObservablePattern Observable Pattern Implementation
 * @{
 */

/**
 * @brief Forward declaration of the Observer template class.
 *
 * @tparam T The type of data observed.
 */
template <typename T> struct Observer;

/**
 * @brief Forward declaration of the Observable template class.
 *
 * @tparam T The type of data being observed.
 */
template <typename T> struct Observable;

/**
 * @brief The Observable class template is the subject being observed.
 *        It notifies its observers about changes.
 *
 * @tparam T The type of data being observed.
 */
template <typename T> struct Observable {
public:
	/**
	 * @brief Type alias for Observer<T> to simplify declarations.
	 */
	typedef Observer<T> Observer;

	/**
	 * @brief Shared pointer type alias for Observer<T> for copying.
	 */
	typedef std::shared_ptr<Observer> pObserver;

	/**
	 * @brief Shared pointer type alias for Observer<T> for moving (identical to pObserver in this context).
	 *        Note: In this implementation, sObserver serves the same purpose as pObserver.
	 *              Typically, for move semantics, a unique_ptr might be more appropriate, but since
	 *              shared_ptr is used here for both, their distinction in this doc mirrors the code.
	 */
	typedef std::shared_ptr<Observer> sObserver;

	/**
	 * @brief Virtual destructor to ensure proper cleanup of derived classes.
	 */
	virtual ~Observable() = default;

	/**
	 * @brief Subscribes an observer to this observable using a shared pointer (copy).
	 *
	 * @param observerCpy Shared pointer to the observer to subscribe.
	 */
	virtual void subscribe(const sObserver& observerCpy) = 0;

	/**
	 * @brief Subscribes an observer to this observable using a shared pointer (move).
	 *
	 * @param observerMov Shared pointer to the observer to subscribe, moved into the observable.
	 */
	virtual void subscribe(sObserver&& observerMov) = 0;

	/**
	 * @brief Unsubscribes an observer from this observable.
	 *
	 * @param observer Shared pointer to the observer to unsubscribe.
	 */
	virtual void unsubscribe(pObserver observer) = 0;
};

/**
 * @brief The Observer class template defines the interface for objects that observe an Observable.
 *
 * @tparam T The type of data being observed.
 */
template <typename T> struct Observer {
public:
	/**
	 * @brief Virtual destructor to ensure proper cleanup of derived classes.
	 */
	virtual ~Observer() = default;

	/**
	 * @brief Called when the observed operation is completed.
	 */
	virtual void onCompleted() = 0;

	/**
	 * @brief Called when an error occurs in the observed operation.
	 *
	 * @param e The exception that occurred.
	 */
	virtual void onError(const std::exception& e) = 0;

	/**
	 * @brief Called when a new value is available from the observed operation.
	 *
	 * @param value The new value.
	 */
	virtual void onNext(const T& value) = 0;
};

/**
 * @brief The Progress class template defines an interface for reporting progress.
 *
 * @tparam T The type of progress value.
 */
template <typename T> struct Progress {
public:
	/**
	 * @brief Virtual destructor to ensure proper cleanup of derived classes.
	 */
	virtual ~Progress() = default;

	/**
	 * @brief Reports the current progress.
	 *
	 * @param value The current progress value.
	 */
	virtual void report(const T& value) = 0;
};

/**
 * @brief The NotifyPropertyChanged interface defines a callback for property changes.
 */
struct NotifyPropertyChanged {
public:
	/**
	 * @brief Virtual destructor to ensure proper cleanup of derived classes.
	 */
	virtual ~NotifyPropertyChanged() = default;

	/**
	 * @brief Called when a property's value changes.
	 *
	 * @param propertyName The name of the property that changed.
	 */
	virtual void onPropertyChanged(const std::string& propertyName) = 0;
};

/** @} */ // End of group ObservablePattern

}
}
#endif // ELVOBSERV_HPP
