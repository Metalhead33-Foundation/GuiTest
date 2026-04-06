#ifndef KALDIOPERATION_HPP
#define KALDIOPERATION_HPP
#include <cstdint>
namespace Kld {

enum class Opcode : uint16_t {};
struct GfxOp {
	Opcode opcode;
	union {} opt;
};

}
#endif // KALDIOPERATION_HPP
