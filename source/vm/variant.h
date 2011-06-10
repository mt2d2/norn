#ifndef VARIANT_H
#define VARIANT_H

union Variant
{
	long l;
	double d;
	char c;
	Variant* p;

	Variant(int i) : l(i) { }
 	Variant(long l) : l(l) { }
	Variant(double d) : d(d) { }
	Variant(char c) : c(c) { }
	Variant(Variant* p) : p(p) { }
	Variant() { }
};

#endif


