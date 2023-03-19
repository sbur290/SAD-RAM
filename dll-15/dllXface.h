#ifndef MIXED_MODE_MULTIPLY_HPP
#define MIXED_MODE_MULTIPLY_HPP
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <iostream>

extern "C"
{
	__declspec(dllexport) int __stdcall addDll(int a) {
		return a + 1;
	}
}

#define ELEMENT_COUNT(a) ((int)(sizeof(a)/sizeof((a)[0])))

#endif //MIXED_MODE_MULTIPLY_HPP...

