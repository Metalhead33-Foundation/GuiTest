#ifndef ELVIOASYNC_HPP
#define ELVIOASYNC_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvAsyncExecutor.hpp>
#include <functional>
#include <memory_resource>
namespace Elv {
namespace Io {

using ByteVector = std::pmr::vector<std::byte>;
using ByteVectorPtr = std::shared_ptr<ByteVector>;

template<Util::AsyncExecutor Executor> struct Async {
	[[nodiscard]] static auto read(Executor& executor, Device& device, void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync( [&device, buffer, size, count] () {
				return device.read(buffer, size, count);
		});
	}
	[[nodiscard]] static auto write(Executor& executor, Device& device, const void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync( [&device, buffer, size, count] () {
				return device.write(buffer, size, count);
		});
	}
	// Note to self: document the resource lifetime rule. `memRes` must outlive the returned value (and obviously the task itself).
	[[nodiscard]] static auto readAll(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([&device, memRes]() {
			const auto pos = static_cast<size_t>(device.tell());
			const auto total = device.size();
			const auto remaining = total > pos ? total - pos : 0;

			std::pmr::vector<std::byte> result(memRes);
			result.resize(remaining);

			if (!result.empty())
				device.read(result.data(), 1, result.size());

			return result;
		});
	}
	// Note to self: document the resource lifetime rule. `memRes` must outlive the returned value (and obviously the task itself).
	[[nodiscard]] static auto readAllShared(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([&device, memRes]() {
			auto result = std::allocate_shared<ByteVector>(
				std::pmr::polymorphic_allocator<ByteVector>(memRes),
				memRes
				);

			const auto pos = static_cast<size_t>(device.tell());
			const auto total = device.size();
			const auto remaining = total > pos ? total - pos : 0;

			result->resize(remaining);

			if (!result->empty())
				device.read(result->data(), 1, result->size());

			return result;
		});
	}
	template<typename Callback> [[nodiscard]] static auto readChunks(Executor& executor, Device& device, size_t chunkSize, Callback&& callback, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync( [&device, memRes, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			std::pmr::vector<std::byte> chunk(memRes);
			chunk.resize(chunkSize);

			while (!device.eof()) {
				const size_t got = device.read(chunk.data(), 1, chunk.size());
				if (got == 0)
					break;
				cb(std::span<const std::byte>(chunk.data(), got));
			}
		});
	}
	template<typename Callback> [[nodiscard]] static auto readChunks(Executor& executor, Device& device, void* chunk, size_t chunkSize, Callback&& callback)
	{
		return executor.enqueueAsync( [&device, chunk, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			while (!device.eof()) {
				const size_t got = device.read(chunk, 1, chunkSize);
				if (got == 0)
					break;
				cb(std::span<const std::byte>(static_cast<std::byte*>(chunk), got));
			}
		});
	}
};

//
}
}
#endif // ELVIOASYNC_HPP
