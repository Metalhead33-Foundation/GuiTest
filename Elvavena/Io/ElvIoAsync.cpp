#include "ElvIoAsync.hpp"
namespace Elv {
namespace Io {

AsyncOperationCanceled::AsyncOperationCanceled()
		: std::runtime_error("Async I/O operation canceled")
	{
	}

}
}