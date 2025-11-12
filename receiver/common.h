#ifndef COMMON_H
#define COMMON_H

#include <windows.h>
#include <string>
#include <sstream>

using namespace std;

const int MAX_MESSAGE_SIZE = 20;

#pragma pack(push,1)

struct FileHeader {
	int capacity; //chislo zapisey (u menya s angliyskim s utra problemy)
	int msg_size;
	int read_index;
	int write_index;
};

#pragma pack(pop)

// "Утилита формирования уникальных имён синхронизаторов по базовому имени файла"
inline string MakeName(const string& base, const string& file) { //eto gpt podskazal, ne osobo ponyal, nado pochitat'
	ostringstream ss;
	ss << "Global\\" << base << "_" << file;
	return ss.str();
}

#include <iostream>
#endif
