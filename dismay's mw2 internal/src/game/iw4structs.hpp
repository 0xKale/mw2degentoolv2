#pragma once

namespace iw4 {

	struct DLCDef {
		int a2;
		const char* name;
	};

	struct DLCList {
		char name[128];
		int a2;
		unsigned char flag1;
		unsigned char flag2;
		unsigned char pad[2];
	};

	static_assert(sizeof(DLCList) == 136, "DLCList stride is 136 bytes innit");
}

