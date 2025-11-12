#ifndef RINGBUFF_H
#define RINGBUFF_H

#include "common.h"
#include <cstring>
#include <string>
//klass ya chastichno pisal s gpt, tk svyazano s tem, chto ya ne ponyal
class RingBuff {
public:
	FileHeader* header;
	char *messages;

	RingBuff(void* base) {
		header = reinterpret_cast<FileHeader*>(base);
		messages = reinterpret_cast<char*>(header + 1);
	}

	void Write(int index, const char* message, int message_size) {
		char* dst = messages + index * message_size;
		memset(dst, 0, message_size);
		strncpy_s(dst, message_size, message, _TRUNCATE);
	}

	string ReadAt(int index, int message_size) {
		char* src = messages + index * message_size;
		src[message_size - 1] = '\0';
		return string(src);
	}
};

#endif // RINGBUFF_H