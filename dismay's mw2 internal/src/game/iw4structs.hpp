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

	struct score_t {
		int clientNum;    // [0]
		int score;        // [1]
		int ping;         // [2]
		int deaths;       // [3]
		int team;         // [4]  TEAM_ALLIES=1, TEAM_AXIS=2, TEAM_SPECTATOR=3
		int kills;        // [5]
		int rank;         // [6]  prestige/rank id from client state
		int assists;      // [7]
		int skill;        // [8]
		int rankIcon;     // [9]  Material* as int level
		int rankIcon2;    // [10] Material* as int prestige
	};
	static_assert(sizeof(score_t) == 44);

	inline constexpr int maxScoreboardClients = 18;
	inline constexpr int cgClientStride = 331;
}

