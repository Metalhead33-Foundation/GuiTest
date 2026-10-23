#ifndef ELVSCOPEGUARD_HPP
#define ELVSCOPEGUARD_HPP
/**
 * @file ElvScopeGuard.hpp
 * @brief Declarative control flow: run an action when a scope exits.
 *
 * The RAII spelling of "do this on the way out", in the style popularised by
 * Andrei Alexandrescu's *Declarative Control Flow* (CppCon 2015). Three
 * statements cover the three exit conditions:
 *
 * @code
 * ELV_SCOPE_EXIT   { unlock(); };                 // every exit
 * ELV_SCOPE_FAIL   { log("failed"); };            // an exception is unwinding
 * ELV_SCOPE_SUCCESS{ commit(); };                 // the scope returns normally
 * @endcode
 *
 * A guard is dismissed with `.dismiss()` when its action should not run, which
 * is the explicit equivalent of `std::lock_guard::release()` or of dismissing a
 * record on the normal path.
 *
 * `ELV_SCOPE_FAIL`/`ELV_SCOPE_SUCCESS` use `std::uncaught_exceptions()`, so they
 * are correct when guards nest: a guard created inside a scope that is already
 * unwinding does not mistake the outer exception for its own.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 */
#include <exception>
#include <type_traits>
#include <utility>

namespace Elv {
namespace Util {

/** @brief When a scope guard runs its action. */
enum class ScopeExitMode {
	Always,    ///< Every exit path.
	OnFailure, ///< Only while an exception unwinds past the guard.
	OnSuccess  ///< Only when the scope completes normally.
};

/**
 * @brief Runs a callable when the guard's scope exits.
 *
 * The callable is stored by value and invoked from the destructor, so it must
 * not throw: a throwing destructor during unwinding terminates the process.
 * The destructor is therefore `noexcept` and the callable is expected to be
 * `noexcept` as well (the macros make the lambda `noexcept`).
 */
template <class Fn>
class ScopeGuard
{
public:
	ScopeGuard(Fn fn, ScopeExitMode mode) noexcept
		: fn_(std::move(fn)),
		  mode_(mode),
		  exceptionsAtConstruction_(std::uncaught_exceptions())
	{
	}

	ScopeGuard(ScopeGuard&& other) noexcept
		: fn_(std::move(other.fn_)),
		  mode_(other.mode_),
		  exceptionsAtConstruction_(other.exceptionsAtConstruction_),
		  active_(other.active_)
	{
		other.active_ = false;
	}

	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;

	~ScopeGuard()
	{
		if (shouldRun())
			fn_();
	}

	/// Prevents the action from running (the normal-path counterpart of a trap).
	void dismiss() noexcept { active_ = false; }

	/// Re-arms a dismissed guard.
	void activate() noexcept { active_ = true; }

	bool active() const noexcept { return active_; }

private:
	bool shouldRun() const noexcept
	{
		if (!active_)
			return false;
		const int now = std::uncaught_exceptions();
		switch (mode_) {
		case ScopeExitMode::Always:
			return true;
		case ScopeExitMode::OnFailure:
			return now > exceptionsAtConstruction_;
		case ScopeExitMode::OnSuccess:
			return now == exceptionsAtConstruction_;
		}
		return false;
	}

	Fn fn_;
	ScopeExitMode mode_;
	int exceptionsAtConstruction_;
	bool active_ = true;
};

/** @brief Creates a guard; prefer the `ELV_SCOPE_*` macros. */
template <class Fn>
[[nodiscard]] ScopeGuard<std::decay_t<Fn>> makeScopeGuard(Fn&& fn,
		ScopeExitMode mode = ScopeExitMode::Always) noexcept
{
	return ScopeGuard<std::decay_t<Fn>>(std::forward<Fn>(fn), mode);
}

/**
 * @brief The left-hand side of the `ELV_SCOPE_*` macros.
 *
 * `atScopeExit(mode) + lambda` builds a guard. The `operator+` spelling is what
 * makes the macro read as a statement that owns the block which follows it:
 *
 *     ELV_SCOPE_EXIT { cleanup(); };
 *
 * expands to `auto guard = atScopeExit() + [&]() noexcept { cleanup(); };`.
 */
struct AtScopeExit {
	ScopeExitMode mode = ScopeExitMode::Always;

	template <class Fn>
	[[nodiscard]] ScopeGuard<std::decay_t<Fn>> operator+(Fn&& fn) const noexcept
	{
		return makeScopeGuard(std::forward<Fn>(fn), mode);
	}
};

/** @brief Builds the guard factory; see `AtScopeExit`. */
inline AtScopeExit atScopeExit(ScopeExitMode mode = ScopeExitMode::Always) noexcept
{
	return AtScopeExit{mode};
}

} // namespace Util
} // namespace Elv

#define ELV_DETAIL_SCOPE_CONCAT_INNER(a, b) a##b
#define ELV_DETAIL_SCOPE_CONCAT(a, b) ELV_DETAIL_SCOPE_CONCAT_INNER(a, b)

/**
 * @def ELV_SCOPE_EXIT
 * @brief Runs the following block on every exit from the enclosing scope.
 *
 * `ELV_SCOPE_EXIT { cleanup(); };`
 */
#define ELV_SCOPE_EXIT \
	auto ELV_DETAIL_SCOPE_CONCAT(elvScopeGuard_, __LINE__) = \
		::Elv::Util::atScopeExit() + [&]() noexcept

/**
 * @def ELV_SCOPE_FAIL
 * @brief Runs the following block only when an exception unwinds out of the scope.
 */
#define ELV_SCOPE_FAIL \
	auto ELV_DETAIL_SCOPE_CONCAT(elvScopeGuardFail_, __LINE__) = \
		::Elv::Util::atScopeExit(::Elv::Util::ScopeExitMode::OnFailure) + [&]() noexcept

/**
 * @def ELV_SCOPE_SUCCESS
 * @brief Runs the following block only when the scope completes normally.
 */
#define ELV_SCOPE_SUCCESS \
	auto ELV_DETAIL_SCOPE_CONCAT(elvScopeGuardSuccess_, __LINE__) = \
		::Elv::Util::atScopeExit(::Elv::Util::ScopeExitMode::OnSuccess) + [&]() noexcept

#endif // ELVSCOPEGUARD_HPP
