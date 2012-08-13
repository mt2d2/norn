#include "opcode.h"

std::map<long, std::string> opcode_str_map()
{
	std::map<long, std::string> m;
#	define op(x) m[x] = #x;
#	include "opcode.def"
#   undef op	

	return m;
}
