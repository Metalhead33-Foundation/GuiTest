#ifndef ELVSPANHELPERS_HPP
#define ELVSPANHELPERS_HPP
#include <span>
#include <cassert>
#include <functional>
#include <glm/glm.hpp>
namespace Elv {
namespace Util {

/**
 * @ingroup ByteSpanWrappers
 * @tparam T Type of the object to be converted into a byte span.
 *
 * @brief Struct containing static functions to convert objects of type T into std::span<std::byte>.
 */
template <typename T> struct span_wrappers {
	/**
	 * @brief Converts an object of type T into a mutable byte span.
	 *
	 * @param thingie Object of type T to be converted.
	 * @return std::span<std::byte> Mutable byte span representing the object's memory.
	 */
	static std::span<std::byte> as_byte_span(T& thingie) {
		return std::span<std::byte>(reinterpret_cast<std::byte*>(&thingie), sizeof(T));
	}

	/**
	 * @brief Converts a const object of type T into a const byte span.
	 *
	 * @param thingie Const object of type T to be converted.
	 * @return const std::span<const std::byte> Constant byte span representing the object's memory.
	 */
	static const std::span<const std::byte> as_const_byte_span(const T& thingie) {
		return std::span<const std::byte>(reinterpret_cast<const std::byte*>(&thingie), sizeof(T));
	}

	/**
	 * @brief Converts an array of objects of type T into a mutable byte span.
	 *
	 * @param thingies Array of objects of type T.
	 * @param size Number of elements in the array.
	 * @return std::span<std::byte> Mutable byte span representing the array's memory.
	 */
	static std::span<std::byte> as_byte_span(T* thingies, size_t size) {
		return std::span<std::byte>(reinterpret_cast<std::byte*>(thingies), sizeof(T) * size);
	}

	/**
	 * @brief Converts a const array of objects of type T into a const byte span.
	 *
	 * @param thingies Const array of objects of type T.
	 * @param size Number of elements in the array.
	 * @return const std::span<const std::byte> Constant byte span representing the array's memory.
	 * @note Corrected reinterpret_cast to directly cast thingies.
	 */
	static const std::span<const std::byte> as_const_byte_span(const T* thingies, size_t size) {
		return std::span<const std::byte>(reinterpret_cast<const std::byte*>(thingies), sizeof(T) * size);
	}

	/**
	 * @brief Converts a std::span of objects of type T into a mutable byte span.
	 *
	 * @param thingies std::span of objects of type T.
	 * @return std::span<std::byte> Mutable byte span representing the span's memory.
	 */
	static std::span<std::byte> as_byte_span(std::span<T>& thingies) {
		return std::span<std::byte>(reinterpret_cast<std::byte*>(thingies.data()), thingies.size_bytes());
	}

	/**
	 * @brief Converts a const std::span of objects of type T into a const byte span.
	 *
	 * @param thingies Const std::span of objects of type T.
	 * @return const std::span<const std::byte> Constant byte span representing the span's memory.
	 */
	static const std::span<const std::byte> as_const_byte_span(const std::span<const T> thingies) {
		return std::span<const std::byte>(reinterpret_cast<const std::byte*>(thingies.data()), thingies.size_bytes());
	}

	/**
	 * @brief Converts a mutable byte span into a mutable span of type T.
	 *
	 * @param byte_span Mutable byte span to be converted.
	 * @return std::span<T> Mutable span of type T.
	 * @pre The byte_span must represent a contiguous block of memory that is correctly aligned for type T.
	 */
	static std::span<T> as_span(std::span<std::byte> byte_span) {
		assert(byte_span.size() % sizeof(T) == 0 && "Byte span size must be a multiple of sizeof(T)");
		assert(reinterpret_cast<std::uintptr_t>(byte_span.data()) % alignof(T) == 0 && "Byte span data must be aligned for type T");
		return std::span<T>(reinterpret_cast<T*>(byte_span.data()), byte_span.size() / sizeof(T));
	}

	/**
	 * @brief Converts a const byte span into a const span of type T.
	 *
	 * @param byte_span Const byte span to be converted.
	 * @return const std::span<const T> Const span of type T.
	 * @pre The byte_span must represent a contiguous block of memory that is correctly aligned for type T.
	 */
	static const std::span<const T> as_const_span(const std::span<const std::byte> byte_span) {
		assert(byte_span.size() % sizeof(T) == 0 && "Byte span size must be a multiple of sizeof(T)");
		assert(reinterpret_cast<std::uintptr_t>(byte_span.data()) % alignof(T) == 0 && "Byte span data must be aligned for type T");
		return std::span<const T>(reinterpret_cast<const T*>(byte_span.data()), byte_span.size() / sizeof(T));
	}

	/**
	 * @brief Iterates over a read-only 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies Const std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the 2D span. The function takes a const reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 */
	static void over_2d_span(const std::span<const T>& thingies, const std::function<void(const T&, const glm::uvec2&)> function, const glm::uvec2& dimensions) {
		for(unsigned y = 0; y < dimensions.y; ++y) {
			const T* const row = &thingies[y*dimensions.x];
			for(unsigned x = 0; x < dimensions.x; ++x) {
				function(row[x], glm::uvec2(x,y));
			}
		}
	}
	/**
	 * @brief Iterates over a specified sub-region of a read-only 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies Const std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the specified sub-region of the 2D span. The function takes a const reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 * @param offset The offset (starting position) of the sub-region within the 2D grid.
	 * @param affected_dimension The dimensions (width and height) of the sub-region to be processed.
	 */
	static void over_2d_span(const std::span<const T>& thingies, const std::function<void(const T&, const glm::uvec2&)> function,
							 const glm::uvec2& dimensions, const glm::uvec2& offset, const glm::uvec2& affected_dimension) {
		const unsigned max_x = std::min(dimensions.x,offset.x+affected_dimension.x);
		const unsigned max_y = std::min(dimensions.y,offset.y+affected_dimension.y);
		for(unsigned y = offset.y; y < max_y; ++y) {
			const T* const row = &thingies[y*dimensions.x];
			for(unsigned x = offset.x; x < max_x; ++x) {
				function(row[x], glm::uvec2(x,y));
			}
		}
	}
	/**
	 * @brief Iterates over a 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the 2D span. The function takes a reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 */
	static void over_2d_span_mut(std::span<T> thingies, const std::function<void(T&, const glm::uvec2&)> function, const glm::uvec2& dimensions) {
		for(unsigned y = 0; y < dimensions.y; ++y) {
			T* const row = &thingies[y*dimensions.x];
			for(unsigned x = 0; x < dimensions.x; ++x) {
				function(row[x], glm::uvec2(x,y));
			}
		}
	}
	/**
	 * @brief Iterates over a specified sub-region of a 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the specified sub-region of the 2D span. The function takes a reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 * @param offset The offset (starting position) of the sub-region within the 2D grid.
	 * @param affected_dimension The dimensions (width and height) of the sub-region to be processed.
	 */
	static void over_2d_span_mut(std::span<T> thingies, const std::function<void(T&, const glm::uvec2&)> function,
							 const glm::uvec2& dimensions, const glm::uvec2& offset, const glm::uvec2& affected_dimension) {
		const unsigned max_x = std::min(dimensions.x,offset.x+affected_dimension.x);
		const unsigned max_y = std::min(dimensions.y,offset.y+affected_dimension.y);
		for(unsigned y = offset.y; y < max_y; ++y) {
			T* const row = &thingies[y*dimensions.x];
			for(unsigned x = offset.x; x < max_x; ++x) {
				function(row[x], glm::uvec2(x,y));
			}
		}
	}
};

/**
 * @ingroup ByteSpanWrappers
 * @tparam Specialization for void type, allowing direct byte manipulation without a specific type T.
 */
template <> struct span_wrappers<void> {
	/**
	 * @brief Converts a void pointer and size into a mutable byte span.
	 *
	 * @param thingies Void pointer to the memory location.
	 * @param size Size in bytes of the memory block.
	 * @return std::span<std::byte> Mutable byte span representing the memory block.
	 */
	static std::span<std::byte> as_byte_span(void* thingies, size_t size) {
		return std::span<std::byte>(reinterpret_cast<std::byte*>(thingies), size);
	}

	/**
	 * @brief Converts a const void pointer and size into a const byte span.
	 *
	 * @param thingies Const void pointer to the memory location.
	 * @param size Size in bytes of the memory block.
	 * @return const std::span<const std::byte> Constant byte span representing the memory block.
	 * @note Corrected to directly cast thingies.
	 */
	static const std::span<const std::byte> as_const_byte_span(const void* thingies, size_t size) {
		return std::span<const std::byte>(reinterpret_cast<const std::byte*>(thingies), size);
	}
};

/**
 * @ingroup ByteSpanWrappers
 * @brief Free function to convert a std::span of T into a mutable byte span.
 *
 * @tparam T Type of the objects in the span.
 * @param thingies std::span of objects of type T.
 * @return std::span<std::byte> Mutable byte span representing the span's memory.
 */
template <typename T> std::span<std::byte> as_byte_span(std::span<T>& thingies) {
	return span_wrappers<T>::as_byte_span(thingies);
}

/**
 * @ingroup ByteSpanWrappers
 * @brief Free function to convert a const std::span of T into a const byte span.
 *
 * @tparam T Type of the objects in the span.
 * @param thingies Const std::span of objects of type T.
 * @return const std::span<const std::byte> Constant byte span representing the span's memory.
 */
template <typename T> const std::span<const std::byte> as_const_byte_span(const std::span<const T> thingies) {
	return span_wrappers<T>::as_const_byte_span(thingies);
}

/**
 * @ingroup ByteSpanWrappers
 * @brief Free function to convert an object of T into a mutable byte span.
 *
 * @tparam T Type of the object.
 * @param thingie Object of type T.
 * @return std::span<std::byte> Mutable byte span representing the object's memory.
 */
template <typename T> std::span<std::byte> as_byte_span(T& thingie) {
	return span_wrappers<T>::as_byte_span(thingie);
}

/**
 * @ingroup ByteSpanWrappers
 * @brief Free function to convert a const object of T into a const byte span.
 *
 * @tparam T Type of the object.
 * @param thingie Const object of type T.
 * @return const std::span<const std::byte> Constant byte span representing the object's memory.
 */
template <typename T> const std::span<const std::byte> as_const_byte_span(const T& thingie) {
	return span_wrappers<T>::as_const_byte_span(thingie);
}

/**
 * @ingroup ByteSpanWrappers
 * @brief Free function to convert an array of T into a mutable byte span.
 *
 * @tparam T Type of the objects in the array.
 * @param thingies Array of objects of type T.
 * @param size Number of elements in the array.
 * @return std::span<std::byte> Mutable byte span representing the array's memory.
 */
template <typename T> std::span<std::byte> as_byte_span(T* thingies, size_t size) {
	return span_wrappers<T>::as_byte_span(thingies, size);
}

/**
 * @ingroup ByteSpanWrappers
 * @brief Free function to convert a const array of T into a const byte span.
 *
 * @tparam T Type of the objects in the array.
 * @param thingies Const array of objects of type T.
 * @param size Number of elements in the array.
 * @return const std::span<const std::byte> Constant byte span representing the array's memory.
 */
template <typename T> const std::span<const std::byte> as_const_byte_span(const T* thingies, size_t size) {
	return span_wrappers<T>::as_const_byte_span(thingies, size);
}


/**
 * @ingroup ByteSpanWrappers
	 * @brief Free function to convert a mutable byte span into a mutable span of type T.
	 *
	 * @tparam T Type of the objects in the array.
	 * @param byte_span Mutable byte span to be converted.
	 * @return std::span<T> Mutable span of type T.
	 * @pre The byte_span must represent a contiguous block of memory that is correctly aligned for type T.
	 */
template <typename T> std::span<T> as_span(std::span<std::byte> byte_span) {
	return span_wrappers<T>::as_span(byte_span);
}

/**
 * @ingroup ByteSpanWrappers
	 * @brief Free function to convert a const byte span into a const span of type T.
	 *
	 * @tparam T Type of the objects in the array.
	 * @param byte_span Const byte span to be converted.
	 * @return const std::span<const T> Const span of type T.
	 * @pre The byte_span must represent a contiguous block of memory that is correctly aligned for type T.
	 */
template <typename T> const std::span<const T> as_const_span(const std::span<const std::byte> byte_span) {
	return span_wrappers<T>::as_const_span(byte_span);
}

/**
	@ingroup DataTransformers
	@tparam T Type of the objects in the array.
	 * @brief Iterates over a read-only 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies Const std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the 2D span. The function takes a const reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 */
template <typename T> void over_2d_span(const std::span<const T>& thingies, const std::function<void(const T&, const glm::uvec2&)> function, const glm::uvec2& dimensions) {
	span_wrappers<T>::over_2d_span(thingies,function,dimensions);
}
/**
	@ingroup DataTransformers
	@tparam T Type of the objects in the array.
	 * @brief Iterates over a specified sub-region of a read-only 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies Const std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the specified sub-region of the 2D span. The function takes a const reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 * @param offset The offset (starting position) of the sub-region within the 2D grid.
	 * @param affected_dimension The dimensions (width and height) of the sub-region to be processed.
	 */
template <typename T> void over_2d_span(const std::span<const T>& thingies, const std::function<void(const T&, const glm::uvec2&)> function,
						 const glm::uvec2& dimensions, const glm::uvec2& offset, const glm::uvec2& affected_dimension) {
	span_wrappers<T>::over_2d_span(thingies,function,dimensions,offset,affected_dimension);
}

/**
	@ingroup DataTransformers
	@tparam T Type of the objects in the array.
	 * @brief Iterates over a 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the 2D span. The function takes a reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 */
template <typename T> void over_2d_span_mut(std::span<T> thingies, const std::function<void(T&, const glm::uvec2&)> function, const glm::uvec2& dimensions) {
	span_wrappers<T>::over_2d_span_mut(thingies,function,dimensions);
}
/**
	@ingroup DataTransformers
	@tparam T Type of the objects in the array.
	 * @brief Iterates over a specified sub-region of a 2D span of objects of type T and applies a given function to each element.
	 *
	 * @param thingies std::span of objects of type T, representing a 2D grid.
	 * @param function A function to be applied to each element in the specified sub-region of the 2D span. The function takes a reference to an object of type T and its 2D position as a glm::uvec2.
	 * @param dimensions The dimensions (width and height) of the 2D grid.
	 * @param offset The offset (starting position) of the sub-region within the 2D grid.
	 * @param affected_dimension The dimensions (width and height) of the sub-region to be processed.
	 */
template <typename T> void over_2d_span_mut(std::span<T> thingies, const std::function<void(T&, const glm::uvec2&)> function,
						 const glm::uvec2& dimensions, const glm::uvec2& offset, const glm::uvec2& affected_dimension) {
	span_wrappers<T>::over_2d_span_mut(thingies,function,dimensions,offset,affected_dimension);
}

inline void over_2d_grid(const std::function<void(unsigned index)> function, const glm::uvec2& dimensions, const glm::uvec2& offset, const glm::uvec2& affected_dimension) {
	const unsigned max_x = std::min(dimensions.x,offset.x+affected_dimension.x);
	const unsigned max_y = std::min(dimensions.y,offset.y+affected_dimension.y);
	for(unsigned y = 0; y < max_y; ++y) {
		const unsigned row = y*dimensions.x;
		for(unsigned x = 0; x < max_x; ++x) {
			function(row+x);
		}
	}
};

/**
 * @ingroup DataTransformers
 * @tparam TIn Input data type.
 * @tparam TOut Output data type.
 *
 * @brief Struct containing functions to transform spans of TIn into spans of TOut.
 */
template <typename TIn, typename TOut> struct transform_wrappers {
	/**
	 * @brief Simple transformation by static_cast.
	 *
	 * @param input Input span of type TIn.
	 * @param output Output span of type TOut, must be the same size as input.
	 */
	void transform(const std::span<const TIn>& input, const std::span<TOut>& output) {
		assert(input.size() == output.size());
		for (size_t i = 0; i < input.size(); ++i) {
			output[i] = static_cast<TOut>(input[i]);
		}
	}

	/**
	 * @typedef tf0_t
	 * @brief Type definition for a transformation function taking a const TIn& and returning TOut.
	 */
	typedef std::function<TOut(const TIn&)> tf0_t;

	/**
	 * @brief Transformation using a provided function of type tf0_t.
	 *
	 * @param input Input span of type TIn.
	 * @param output Output span of type TOut, must be the same size as input.
	 * @param transformFun Transformation function to apply to each element.
	 */
	void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const tf0_t& transformFun) {
		assert(input.size() == output.size());
		for (size_t i = 0; i < input.size(); ++i) {
			output[i] = transformFun(input[i]);
		}
	}

	/**
	 * @typedef tf1_t
	 * @brief Type definition for a transformation function taking a const TIn& and a TOut& for output.
	 */
	typedef std::function<void(const TIn&, TOut&)> tf1_t;

	/**
	 * @brief Transformation using a provided function of type tf1_t.
	 *
	 * @param input Input span of type TIn.
	 * @param output Output span of type TOut, must be the same size as input.
	 * @param transformFun Transformation function to apply to each element.
	 */
	void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const tf1_t& transformFun) {
		assert(input.size() == output.size());
		for (size_t i = 0; i < input.size(); ++i) {
			transformFun(input[i], output[i]);
		}
	}
};

/**
 * @ingroup DataTransformers
 * @brief Free function to transform a span of TIn into a span of TOut using static_cast.
 *
 * @tparam TIn Input data type.
 * @tparam TOut Output data type.
 * @param input Input span of type TIn.
 * @param output Output span of type TOut, must be the same size as input.
 */
template <typename TIn, typename TOut> void transform(const std::span<const TIn>& input, const std::span<TOut>& output) {
	transform_wrappers<TIn, TOut>::transform(input, output);
}

/**
 * @ingroup DataTransformers
 * @brief Free function to transform a span of TIn into a span of TOut using a provided transformation function (returning TOut).
 *
 * @tparam TIn Input data type.
 * @tparam TOut Output data type.
 * @param input Input span of type TIn.
 * @param output Output span of type TOut, must be the same size as input.
 * @param transFormFun Transformation function (TIn -> TOut).
 */
template <typename TIn, typename TOut> void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const std::function<TOut(const TIn&)>& transFormFun) {
	transform_wrappers<TIn, TOut>::transform(input, output, transFormFun);
}

/**
 * @ingroup DataTransformers
 * @brief Free function to transform a span of TIn into a span of TOut using a provided transformation function (taking TIn& and TOut&).
 *
 * @tparam TIn Input data type.
 * @tparam TOut Output data type.
 * @param input Input span of type TIn.
 * @param output Output span of type TOut, must be the same size as input.
 * @param transFormFun Transformation function (TIn&, TOut&).
 */
template <typename TIn, typename TOut> void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const std::function<void(const TIn&, TOut&)>& transFormFun) {
	transform_wrappers<TIn, TOut>::transform(input, output, transFormFun);
}

}
}
#endif // ELVSPANHELPERS_HPP
