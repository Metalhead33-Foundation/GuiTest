#ifndef MHOBSERV_HPP
#define MHOBSERV_HPP
#include <memory>
#include <exception>

namespace MH33 {
namespace Interface {

/*! An observable obejct.*/
/*!
  \tparam T The type of data examined in the observation process.
*/
template <typename T> class Observable;
/*! An observing obejct.*/
/*!
  \tparam T The type of data examined in the observation process.
*/
template <typename T> class Observer;

/*! An observable obejct.*/
/*!
  \tparam T The type of data examined in the observation process.
*/
template <typename T> class Observable {
public:
	/*! A shared pointer to the observer.*/
    typedef std::shared_ptr<Observer<T>> sObserver;
	/*! Destructor.*/
    virtual ~Observable() = default;
	/*! Subscribe to an observer.*/
	/*!
	  \param observer The observer to subscribe to.
	*/
    virtual void subscribe(const sObserver& observer) = 0;
	/*! Subscribe to an observer.*/
	/*!
	  \param observer The observer to subscribe to.
	*/
    virtual void subscribe(sObserver&& observer) = 0;
	/*! Unsubscribe fron an observer.*/
	/*!
	  \param observer The observer to unsubscribe from.
	*/
    virtual void unsubscribe(Observer<T>* observer) = 0;
	/*! Unsubscribe fron an observer.*/
	/*!
	  \param observer The observer to unsubscribe from.
	*/
	inline void unsubscribe(const sObserver& observer) {
	unsubscribe(observer.get());
    }
	/*! Unsubscribe fron an observer.*/
	/*!
	  \param observer The observer to unsubscribe from.
	*/
	inline void unsubscribe(const Observer<T>& observer) {
	unsubscribe(&observer);
    }
};

/*! An observing object.*/
/*!
  \tparam T The type of data examined in the observation process.
*/
template <typename T> class Observer {
public:
	/*! Destructor.*/
    virtual ~Observer() = default;
	/*! An observable object.*/
    typedef Observable<T> ObservableType;
	/*! Called when an observable object will not transmit any more data.*/
    virtual void onFinished() = 0;
	/*! Called when an observable object has encountered an error.*/
	/*!
	  \param error The error that was encountered.
	*/
    virtual void onError(const std::exception& error) = 0;
	/*! Called when an observable object is transmitting data.*/
	/*!
	  \param data The data being transmitted.
	*/
    virtual void onNext(const T& data) = 0;
};

}
}

#endif // MHOBSERV_HPP
