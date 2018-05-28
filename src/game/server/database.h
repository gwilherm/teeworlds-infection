#include <engine/external/sqlite/sqlite3.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>

#include "entities/pickup.h"
#include <engine/console.h>
#include <engine/shared/config.h>

class CDatabase 
{
	bool m_runControlThread;
	bool m_databaseFree;
	sqlite3 *db;
	int rc;

	std::string m_Name;
	int m_Kills;
	int m_KillsAsZombie;
	
	struct Player{
		int id;
		std::string Name;
        int Kills;
        int KillsAsZombie;;
		int AvgPing;
		int TimeInGame;
		bool WonAsHuman;
		int Score;
		int Death;
		float Distance;
    };
	
	Player m_aPlayers[MAX_CLIENTS];
	
	std::vector<std::string> vStats;

	class IConsole *m_pConsole;
	
	void taskAddRoundStats(int id, std::string Name, int Kills, int KillsAsZombie, int AvgPing, int TimeInGame, bool WonAsHuman, int Score, int Death, float Distance);
	void taskRoundStats();
	void taskPlayerStats(std::string RequestedBy, std::string Name);
	void taskTop5(std::string RequestedBy, std::string Name);
	
	std::vector<std::thread*> m_RankingThreads;
	std::thread* m_ControlThread;
	
	static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to); 

public:
	CDatabase(IConsole *pConsole);
	~CDatabase();
	
	void AddRoundStats(int id, std::string Name, int Kills, int KillsAsZombie, int AvgPing, int TimeInGame, bool WonAsHuman, int Score, int Death, float Distance);
	void NewRound();
	void RoundStats();
	void PlayerStats(std::string RequestedBy, std::string Name);
	void Top5(std::string RequestedBy, std::string Name);
	std::vector<std::string> GetStats();
	
	static int callback(void *unused, int count, char **data, char **columns);
};