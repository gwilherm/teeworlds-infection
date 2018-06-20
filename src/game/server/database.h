#include <engine/external/sqlite/sqlite3.h>
#include <string>
#include <queue>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <mutex>

#include "entities/pickup.h"
#include <engine/console.h>
#include <engine/shared/config.h>

class CDatabase 
{
	bool m_runControlThread;
	std::mutex m_mutexDatabaseFree;
	
	sqlite3 *db;

	struct Player{
		int id;
		std::string Name;
        int KillsWeapons;
		int KillsBarrier;
        int KillsAsZombie;
		int AvgPing;
		int TimeInGame;
		bool WonAsHuman;
		int Score;
		int Death;
		float Distance;
    };
	
	Player m_aPlayers[MAX_CLIENTS];
	
	std::queue<std::string> qStats;

	class IConsole *m_pConsole;
	
	void taskAddRoundStats(int id, std::string Name, int KillsWeapons, int KillsBarrier, int KillsAsZombie, int AvgPing, int TimeInGame, bool WonAsHuman, int Score, int Death, float Distance);
	void taskPlayerStats(std::string RequestedBy, std::string Name);
	void taskTop5(std::string RequestedBy, std::string Name);
	
	std::queue<std::thread*> m_RankingThreads;
	std::thread* m_ControlThread;
	
	static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to); 
	static int callbackRank(void *unused, int count, char **data, char **columns);
	static int callbackTop5(void *unused, int count, char **data, char **columns);
	
public:
	CDatabase(IConsole *pConsole);
	~CDatabase();
	
	void AddRoundStats(int id, std::string Name, int KillsWeapons, int KillsBarrier, int KillsAsZombie, int AvgPing, int TimeInGame, bool WonAsHuman, int Score, int Death, float Distance);
	void NewRound();
	void RoundStats();
	void PlayerStats(std::string RequestedBy, std::string Name);
	void Top5(std::string RequestedBy, std::string Name);
	std::queue<std::string> GetStats();
	

};
