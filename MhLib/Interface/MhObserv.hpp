#ifndef MHOBSERV_HPP
#define MHOBSERV_HPP
#include <memory>
#include <exception>

namespace MH33 {
namespace Interface {

template <typename T> class Observable;
template <typename T> class Observer;

template <typename T> class Observable {
public:
    typedef std::shared_ptr<Observer<T>> sObserver;
    virtual ~Observable() = default;
    virtual void subscribe(const sObserver& observer) = 0;
    virtual void subscribe(sObserver&& observer) = 0;
    virtual void unsubscribe(Observer<T>* observer) = 0;
	inline void unsubscribe(const sObserver& observer) {
	unsubscribe(observer.get());
    }
	inline void unsubscribe(const Observer<T>& observer) {
	unsubscribe(&observer);
    }
};

template <typename T> class Observer {
public:
    virtual ~Observer() = default;
    typedef Observable<T> ObservableType;
    virtual void onFinished() = 0;
    virtual void onError(const std::exception& error) = 0;
    virtual void onNext(const T& data) = 0;
};

}
}

#endif // MHOBSERV_HPP
