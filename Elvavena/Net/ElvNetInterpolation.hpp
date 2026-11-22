#ifndef ELVNETINTERPOLATION_HPP
#define ELVNETINTERPOLATION_HPP
/**
 * @file ElvNetInterpolation.hpp
 * @brief Interpolation hooks and snapshot jitter buffers.
 *
 * `InterpolationOps` is a function-pointer table, matching the rest of the
 * netcode hot paths. `makeInterpolationOps<T>()` adapts arithmetic types that
 * satisfy `Elv::Util::Interpolable`; schema-based games can use
 * `schemaInterpolationOps()` with explicit per-field rules (float/int/angle).
 *
 * `JitterBuffer<T>` stores timestamped values out of order and samples between
 * the two neighbours. `SnapshotJitterBuffer` does the same for encoded snapshot
 * payloads, leaving component decoding to `SnapshotReader`.
 */
#include <Elvavena/Io/ElvDataSerializationSchema.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <Elvavena/Util/ElvInterpolation.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @struct InterpolationOps
 * @brief Type-erased interpolation callback.
 *
 * `out`, `a` and `b` point to objects/component memory of the type the table was
 * built for. `alpha` is in [0, 1].
 */
struct InterpolationOps {
	/**
	 * @brief Interpolates `a` and `b` into `out`.
	 * @param out Destination object.
	 * @param a First sample.
	 * @param b Second sample.
	 * @param alpha Weight of `b`, in [0, 1].
	 * @param context Ops-specific context.
	 * @return True on success.
	 */
	bool (*interpolate)(void* out, const void* a, const void* b, float alpha,
		const void* context);
	const void* context = nullptr; ///< Ops-specific context.
};

/**
 * @brief Creates an interpolation table for an arithmetic/interpolable type.
 * @tparam T Type supporting `Util::Interpolation<T, float>`.
 * @return Static ops table.
 */
template <typename T>
	requires Util::Interpolable<T, float>
const InterpolationOps* makeInterpolationOps() noexcept {
	static const InterpolationOps ops {
		[](void* out, const void* a, const void* b, float alpha, const void*) {
			*static_cast<T*>(out) = Util::Interpolation<T, float>::lerp(
				*static_cast<const T*>(a), *static_cast<const T*>(b), alpha);
			return true;
		},
		nullptr
	};
	return &ops;
}

/**
 * @enum SchemaInterpolationKind
 * @brief How one schema field is interpolated.
 */
enum class SchemaInterpolationKind : std::uint8_t {
	Skip,  ///< Copy the latest sample.
	Float, ///< Interpolate float/double fields.
	Int,   ///< Interpolate integer fields and round.
	Angle, ///< Interpolate float angles along the shortest arc.
};

/** @struct SchemaInterpolationRule
 *  @brief Per-field interpolation rule.
 */
struct SchemaInterpolationRule {
	std::uint16_t fieldIndex = 0;
	SchemaInterpolationKind kind = SchemaInterpolationKind::Skip;
};

/** @struct SchemaInterpolationDesc
 *  @brief Schema plus field rules used by `schemaInterpolationOps`.
 */
struct SchemaInterpolationDesc {
	const Io::ObjectDefinition* schema = nullptr;
	const SchemaInterpolationRule* rules = nullptr;
	std::size_t ruleCount = 0;
};

namespace detail {

/** @brief Reads a field of the requested width as a signed integer. */
inline std::int64_t readInteger(const void* data, Io::DataType type) {
	switch (type) {
	case Io::DataType::INT8: {
		std::int8_t value = 0;
		std::memcpy(&value, data, sizeof(value));
		return value;
	}
	case Io::DataType::INT16: {
		std::int16_t value = 0;
		std::memcpy(&value, data, sizeof(value));
		return value;
	}
	case Io::DataType::INT32: {
		std::int32_t value = 0;
		std::memcpy(&value, data, sizeof(value));
		return value;
	}
	case Io::DataType::INT64: {
		std::int64_t value = 0;
		std::memcpy(&value, data, sizeof(value));
		return value;
	}
	default:
		return 0;
	}
}

/** @brief Writes a signed integer into a field of the requested width. */
inline void writeInteger(void* data, Io::DataType type, std::int64_t value) {
	switch (type) {
	case Io::DataType::INT8: {
		const auto narrowed = static_cast<std::int8_t>(value);
		std::memcpy(data, &narrowed, sizeof(narrowed));
		break;
	}
	case Io::DataType::INT16: {
		const auto narrowed = static_cast<std::int16_t>(value);
		std::memcpy(data, &narrowed, sizeof(narrowed));
		break;
	}
	case Io::DataType::INT32: {
		const auto narrowed = static_cast<std::int32_t>(value);
		std::memcpy(data, &narrowed, sizeof(narrowed));
		break;
	}
	case Io::DataType::INT64:
		std::memcpy(data, &value, sizeof(value));
		break;
	default:
		break;
	}
}

/** @brief Interpolates fields according to a schema description. */
inline bool schemaInterpolate(void* out, const void* a, const void* b, float alpha,
	const void* context) {
	const auto* desc = static_cast<const SchemaInterpolationDesc*>(context);
	if (desc == nullptr || desc->schema == nullptr || out == nullptr || a == nullptr || b == nullptr) {
		return false;
	}
	const Io::ObjectDefinition& schema = *desc->schema;
	std::memcpy(out, b, schema.stride); // Latest non-interpolated fields.
	for (std::size_t index = 0; index < desc->ruleCount; ++index) {
		const SchemaInterpolationRule& rule = desc->rules[index];
		if (rule.kind == SchemaInterpolationKind::Skip || rule.fieldIndex >= schema.fieldCount) continue;
		const Io::ObjectField& field = schema.fields[rule.fieldIndex];
		if (field.typeDef == nullptr || field.offset >= schema.stride) continue;
		const auto* first = static_cast<const std::byte*>(a) + field.offset;
		const auto* second = static_cast<const std::byte*>(b) + field.offset;
		auto* destination = static_cast<std::byte*>(out) + field.offset;
		if (rule.kind == SchemaInterpolationKind::Float) {
			if (field.typeDef->dataType == Io::DataType::INT32) {
				float lhs = 0.0f;
				float rhs = 0.0f;
				std::memcpy(&lhs, first, sizeof(lhs));
				std::memcpy(&rhs, second, sizeof(rhs));
				const float value = lhs + (rhs - lhs) * alpha;
				std::memcpy(destination, &value, sizeof(value));
			} else if (field.typeDef->dataType == Io::DataType::INT64) {
				double lhs = 0.0;
				double rhs = 0.0;
				std::memcpy(&lhs, first, sizeof(lhs));
				std::memcpy(&rhs, second, sizeof(rhs));
				const double value = lhs + (rhs - lhs) * static_cast<double>(alpha);
				std::memcpy(destination, &value, sizeof(value));
			}
		} else if (rule.kind == SchemaInterpolationKind::Int) {
			const double lhs = static_cast<double>(readInteger(first, field.typeDef->dataType));
			const double rhs = static_cast<double>(readInteger(second, field.typeDef->dataType));
			writeInteger(destination, field.typeDef->dataType,
				static_cast<std::int64_t>(std::llround(lhs + (rhs - lhs) * alpha)));
		} else if (rule.kind == SchemaInterpolationKind::Angle
			&& field.typeDef->dataType == Io::DataType::INT32) {
			float lhs = 0.0f;
			float rhs = 0.0f;
			std::memcpy(&lhs, first, sizeof(lhs));
			std::memcpy(&rhs, second, sizeof(rhs));
			float difference = rhs - lhs;
			constexpr float tau = 6.28318530717958647692f;
			difference -= std::round(difference / tau) * tau;
			const float value = lhs + difference * alpha;
			std::memcpy(destination, &value, sizeof(value));
		}
	}
	return true;
}

} // namespace detail

/**
 * @brief Creates interpolation ops from explicit schema field rules.
 * @param desc Schema and rules; must outlive the returned ops.
 * @return Ops table referencing @p desc.
 */
inline InterpolationOps schemaInterpolationOps(const SchemaInterpolationDesc& desc) noexcept {
	return InterpolationOps { &detail::schemaInterpolate, &desc };
}


/**
 * @class JitterBuffer
 * @brief Ordered tick samples with capacity-bounded overwrite and interpolation.
 * @tparam T Sample value type.
 */
template <typename T>
class JitterBuffer {
public:
	/** @struct Sample
	 *  @brief Timestamped value.
	 */
	struct Sample {
		Tick tick = 0;
		T value {};
	};

	/**
	 * @brief Creates a buffer.
	 * @param capacity Maximum samples; zero rejects inserts.
	 */
	explicit JitterBuffer(std::size_t capacity = 64) : capacity_(capacity) {}

	/**
	 * @brief Inserts or replaces a sample, preserving tick order.
	 * @param tick Sample tick.
	 * @param value Sample value.
	 * @return True on success.
	 */
	bool push(Tick tick, const T& value) {
		if (capacity_ == 0) return false;
		const auto position = std::lower_bound(samples_.begin(), samples_.end(), tick,
			[](const Sample& sample, Tick candidate) { return sample.tick < candidate; });
		if (position != samples_.end() && position->tick == tick) {
			position->value = value;
			return true;
		}
		samples_.insert(position, Sample { tick, value });
		if (samples_.size() > capacity_) samples_.erase(samples_.begin());
		return true;
	}

	/**
	 * @brief Samples at a tick, clamping outside the stored range.
	 * @param tick Target tick.
	 * @param out Receives the sample.
	 * @param ops Optional interpolation table.
	 * @return True when a sample was produced.
	 */
	bool sample(Tick tick, T& out, const InterpolationOps* ops = nullptr) const {
		if (samples_.empty()) return false;
		if (tick <= samples_.front().tick) {
			out = samples_.front().value;
			return true;
		}
		if (tick >= samples_.back().tick) {
			out = samples_.back().value;
			return true;
		}
		const auto next = std::upper_bound(samples_.begin(), samples_.end(), tick,
			[](Tick candidate, const Sample& sample) { return candidate < sample.tick; });
		const auto previous = next - 1;
		const float alpha = static_cast<float>(tick - previous->tick)
			/ static_cast<float>(next->tick - previous->tick);
		if (ops != nullptr && ops->interpolate != nullptr) {
			return ops->interpolate(&out, &previous->value, &next->value, alpha, ops->context);
		}
		out = alpha < 0.5f ? previous->value : next->value;
		return true;
	}

	/** @brief Returns the newest sample. */
	[[nodiscard]] bool latest(T& out) const {
		if (samples_.empty()) return false;
		out = samples_.back().value;
		return true;
	}

	/** @brief Returns the oldest sample. */
	[[nodiscard]] bool oldest(T& out) const {
		if (samples_.empty()) return false;
		out = samples_.front().value;
		return true;
	}

	/** @brief Returns stored samples in tick order. */
	[[nodiscard]] std::span<const Sample> samples() const noexcept { return samples_; }
	/** @brief Returns the sample count. */
	[[nodiscard]] std::size_t size() const noexcept { return samples_.size(); }
	/** @brief Returns the capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
	/** @brief Removes all samples. */
	void clear() noexcept { samples_.clear(); }

private:
	std::vector<Sample> samples_;
	std::size_t capacity_ = 64;
};

/**
 * @struct SnapshotFrame
 * @brief One encoded snapshot payload in a jitter buffer.
 */
struct SnapshotFrame {
	std::uint32_t tick = 0;
	std::vector<std::byte> payload;
};

/**
 * @class SnapshotJitterBuffer
 * @brief Tick-ordered snapshot payload buffer with capacity overwrite.
 */
class SnapshotJitterBuffer {
public:
	/** @brief Creates a buffer with a payload count capacity. */
	explicit SnapshotJitterBuffer(std::size_t capacity = 32) : capacity_(capacity) {}

	/**
	 * @brief Inserts or replaces a snapshot payload.
	 * @param tick Snapshot tick.
	 * @param payload Wire bytes.
	 * @return True on success.
	 */
	bool push(std::uint32_t tick, std::span<const std::byte> payload) {
		if (capacity_ == 0) return false;
		const auto position = std::lower_bound(frames_.begin(), frames_.end(), tick,
			[](const SnapshotFrame& frame, std::uint32_t candidate) { return frame.tick < candidate; });
		SnapshotFrame frame { tick, std::vector<std::byte>(payload.begin(), payload.end()) };
		if (position != frames_.end() && position->tick == tick) {
			*position = std::move(frame);
			return true;
		}
		frames_.insert(position, std::move(frame));
		if (frames_.size() > capacity_) frames_.erase(frames_.begin());
		return true;
	}

	/** @brief Returns the newest frame. */
	[[nodiscard]] const SnapshotFrame* latest() const noexcept {
		return frames_.empty() ? nullptr : &frames_.back();
	}

	/** @brief Returns the oldest frame. */
	[[nodiscard]] const SnapshotFrame* oldest() const noexcept {
		return frames_.empty() ? nullptr : &frames_.front();
	}

	/** @brief Returns the newest frame at or before @p tick. */
	[[nodiscard]] const SnapshotFrame* atOrBefore(std::uint32_t tick) const noexcept {
		const auto position = std::upper_bound(frames_.begin(), frames_.end(), tick,
			[](std::uint32_t candidate, const SnapshotFrame& frame) { return candidate < frame.tick; });
		return position == frames_.begin() ? nullptr : &*(position - 1);
	}

	/** @brief Returns the oldest frame at or after @p tick. */
	[[nodiscard]] const SnapshotFrame* atOrAfter(std::uint32_t tick) const noexcept {
		const auto position = std::lower_bound(frames_.begin(), frames_.end(), tick,
			[](const SnapshotFrame& frame, std::uint32_t candidate) { return frame.tick < candidate; });
		return position == frames_.end() ? nullptr : &*position;
	}

	/** @brief Returns the frame count. */
	[[nodiscard]] std::size_t size() const noexcept { return frames_.size(); }
	/** @brief Returns the capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
	/** @brief Removes all frames. */
	void clear() noexcept { frames_.clear(); }

private:
	std::vector<SnapshotFrame> frames_;
	std::size_t capacity_ = 32;
};

} // namespace Net
} // namespace Elv
#endif // ELVNETINTERPOLATION_HPP

