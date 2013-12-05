#ifndef VARIANT_H
#define VARIANT_H

#include <cstdint> 

union Variant
{
	int64_t l;
	double d;
	char c;
	Variant* p;

	Variant(int64_t i) : l(i) { }
	Variant(int i) : l(i) { }
 	Variant(long l) : l(l) { }
	Variant(double d) : d(d) { }
	Variant(char c) : c(c) { }
	Variant(Variant* p) : p(p) { }
	Variant() { }
};

#endif


