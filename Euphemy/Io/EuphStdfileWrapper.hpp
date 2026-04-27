#ifndef STDFILEWRAPPER_HPP
#define STDFILEWRAPPER_HPP
/**
 * @file EuphStdfileWrapper.hpp
 * @brief Declares the EuphStdfileWrapper API in the Euphemy/Io module.
 *
 * This header is part of the public declaration surface for Euphemy/Io.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstdio>
namespace Euph {
namespace Io {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(StdfileWrapper)
/**
 * @class StdfileWrapper
 * @brief A wrapper class for standard C file streams (FILE*),
 *		implementing the Elv::Io::Device interface.
 *
 * This class provides a bridge between the C standard library's file handling
 * (FILE*) and the Elv::Io::Device abstract interface, enabling the use of
 * standard input/output streams within the Elv::Io framework.
 */
class MH_EUPH_API StdfileWrapper : public Elv::Io::Device
{
private:
	/**
	 * @var cfile
	 * @brief The underlying C standard library file stream.
	 */
	FILE* cfile;

	/**
	 * @var mode
	 * @brief The mode in which the file was opened (e.g., read, write, append).
	 *
	 * @see Elv::Io::Mode
	 */
	Elv::Io::Mode mode;

public:
	/**
	 * @brief Constructs a StdfileWrapper from an existing C file stream and mode.
	 *
	 * @param cfile The C standard library file stream to wrap.
	 * @param mode  The mode in which the file was opened.
	 */
	StdfileWrapper(FILE* cfile, Elv::Io::Mode mode);

	/**
	 * @brief Copy constructor for StdfileWrapper.
	 *
	 * @param cpy The StdfileWrapper instance to copy.
	 *
	 * @note This may involve duplicating the underlying file descriptor,
	 *	   depending on the implementation.
	 */
	StdfileWrapper(const StdfileWrapper& cpy);

	/**
	 * @brief Assignment operator for StdfileWrapper.
	 *
	 * @param cpy The StdfileWrapper instance to assign from.
	 *
	 * @return A reference to the assigned StdfileWrapper instance.
	 *
	 * @note The behavior of this operator may depend on the specific requirements
	 *	   of the application, such as whether to duplicate or share the
	 *	   underlying file descriptor.
	 */
	StdfileWrapper& operator=(const StdfileWrapper& cpy);

	/**
	 * @brief Reads data from the wrapped file stream into a buffer.
	 *
	 * @param buffer Pointer to the buffer to fill with data from the file.
	 * @param size   Size of each element to read.
	 * @param count  Number of elements to read.
	 *
	 * @return The number of elements successfully read.
	 *
	 * @note Implements `Elv::Io::Device::read`.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data from a buffer to the wrapped file stream.
	 *
	 * @param buffer Pointer to the buffer containing data to write.
	 * @param size   Size of each element to write.
	 * @param count  Number of elements to write.
	 *
	 * @return The number of elements successfully written.
	 *
	 * @note Implements `Elv::Io::Device::write`.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Seeks to a position in the wrapped file stream.
	 *
	 * @param offset The number of bytes to offset from the origin.
	 * @param whence The origin from which to seek (beginning, current, end).
	 *
	 * @return 0 upon success, non-zero on failure.
	 *
	 * @note Implements `Elv::Io::Device::seek`.
	 * @see Elv::Io::SeekOrigin
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current position in the wrapped file stream.
	 *
	 * @return The current position in bytes from the beginning of the file.
	 *
	 * @note Implements `Elv::Io::Device::tell`.
	 */
	long tell() override;

	/**
	 * @brief Retrieves the total size of the wrapped file.
	 *
	 * @return The size of the file in bytes.
	 *
	 * @note Implements `Elv::Io::Device::size`.
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the wrapped file has been reached.
	 *
	 * @return True if the end of the file has been reached, false otherwise.
	 *
	 * @note Implements `Elv::Io::Device::eof`.
	 */
	bool eof() override;

	/**
	 * @brief Retrieves the access mode of the wrapped file.
	 *
	 * @return The mode in which the file was opened (read, write, append, etc.).
	 *
	 * @note Implements `Elv::Io::Device::getMode`.
	 * @see Elv::Io::Mode
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes the wrapped file stream to ensure written data is committed.
	 *
	 * @return True if the flush operation was successful, false otherwise.
	 *
	 * @note Implements `Elv::Io::Device::flush`.
	 */
	bool flush() override;

	/**
	 * @brief Checks if the wrapped file stream is in a valid state.
	 *
	 * @return True if the file stream is valid, false otherwise.
	 *
	 * @note Implements `Elv::Io::Device::isValid`.
	 */
	bool isValid() const override;
};

/**
 * @brief Set `NODECLARE_STANDARD_OUTPUTS` to suppress declaration of wrapped std streams.
 *
 * If defined, the following variables will not be declared:
 * - STDIN
 * - STDOUT
 * - STDERR
 */

#ifndef NODECLARE_STANDARD_OUTPUTS
/**
 * @var STDIN
 * @brief A StdfileWrapper instance for the standard input stream.
 */
extern StdfileWrapper STDIN;

/**
 * @var STDOUT
 * @brief A StdfileWrapper instance for the standard output stream.
 */
extern StdfileWrapper STDOUT;

/**
 * @var STDERR
 * @brief A StdfileWrapper instance for the standard error stream.
 */
extern StdfileWrapper STDERR;
#endif

}
}
#endif // STDFILEWRAPPER_HPP
