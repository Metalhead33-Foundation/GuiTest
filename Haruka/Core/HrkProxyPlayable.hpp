#ifndef HRKPROXYPLAYABLE_HPP
#define HRKPROXYPLAYABLE_HPP
/**
 * @file HrkProxyPlayable.hpp
 * @brief Declares the HrkProxyPlayable API in the Haruka/Core module.
 *
 * This header is part of the public declaration surface for Haruka/Core.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/Core/HrkPlayable.hpp>
#include <cstddef>
#include <memory>
namespace Hrk {

/** @brief Base class for playables that forward to another playable. */
class MH_HARUKA_API ProxyPlayable : public Playable
{
protected:
	/** @brief Documents the playable declaration. */
	Playable* playable = nullptr;
	/** @brief Documents the ownedPlayable declaration. */
	std::shared_ptr<Playable> ownedPlayable;
public:
	/** @brief Returns the current target playable, or null when unset. */
	const Playable* getPlayable() const;
	/** @brief Returns the current mutable target playable, or null when unset. */
	Playable* getPlayable();
	/**
	 * @brief Set a non-owning playable target.
	 *
	 * The caller is responsible for the target lifetime. This keeps stack allocation possible.
	 */
	void setPlayable(Playable* newPlayable);
	/**
	 * @brief Set an owning playable target.
	 *
	 * The proxy keeps shared ownership and forwards to the managed instance.
	 */
	void setPlayable(std::shared_ptr<Playable> newPlayable);
	/// @brief Clear any target playable and release owned state.
	void setPlayable(std::nullptr_t);
};

}
#endif // HRKPROXYPLAYABLE_HPP
