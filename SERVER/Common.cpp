#include "stdafx.h"
#include "Common.h"
#include "CONST.h"
#include <list>

string CreateRamdomID() {
	int length = rand() % 6 + 5;
	string result = "";
	srand((int)time(0));
	for (int i = 0; i < length; i++) {
		int element = rand() % 26 + 97;
		result += (char)element;
	}
	return result;
}