#ifndef HRKPROXYPLAYABLE_HPP
#define HRKPROXYPLAYABLE_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <cstddef>
#include <memory>
namespace Hrk {

class MH_HARUKA_API ProxyPlayable : public Playable
{
protected:
	Playable* playable = nullptr;
	std::shared_ptr<Playable> ownedPlayable;
public:
	const Playable* getPlayable() const;
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
