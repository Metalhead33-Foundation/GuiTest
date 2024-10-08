#ifndef ELVOBSERV_HPP
#define ELVOBSERV_HPP
#include <memory>
#include <exception>
namespace Elv {
namespace Util {

template <typename T> struct Observer;
template <typename T> struct Observable;

template <typename T> struct Observable {
public:
	typedef Observer<T> Observer;
	typedef std::shared_ptr<Observer> pObserver;
	typedef std::shared_ptr<Observer> sObserver;
	virtual ~Observable() = default;
	virtual void subscribe(const sObserver& observerCpy) = 0;
	virtual void subscribe(sObserver&& observerMov) = 0;
	virtual void unsubscribe(pObserver observer) = 0;
};
template <typename T> struct Observer {
public:
	virtual ~Observer() = default;
	virtual void onCompleted() = 0;
	virtual void onError(const std::exception& e) = 0;
	virtual void onNext(const T& value) = 0;
};

template <typename T> struct Progress {
	virtual ~Progress() = default;
	virtual void report(const T& value) = 0;
};

struct NotifyPropertyChanged {
	virtual ~NotifyPropertyChanged() = default;
	virtual void onPropertyChanged(const std::string& propertyName) = 0;
};

}
}
#endif // ELVOBSERV_HPP
