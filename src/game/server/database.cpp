#include <engine/console.h>
#include "database.h"
#include <iomanip>
#include <sstream> 

CDatabase::CDatabase(IConsole *pConsole)
{
	if(g_Config.m_SvDatabase){
		m_runControlThread = true;		
		m_ControlThread = new std::thread([=]() {
			while(m_runControlThread){
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
		
				for(size_t i = 0; i< m_RankingThreads.size(); ++i){
					if(m_RankingThreads[i]->joinable())
					{
						m_RankingThreads[i]->join();
						std::thread * tmp = m_RankingThreads[i];
						delete tmp;
						m_RankingThreads.erase(m_RankingThreads.begin()+i);
						
					}
				}
			}			
		});
	}
	m_pConsole = pConsole;

	char *zErrMsg = 0;
	/*		OPEN/CREATE DATABASE	*/
	rc = sqlite3_open("stats.db", &db);
   
	std::string status; 
	if( rc ) {
		status = "Can't open database: " + std::string(sqlite3_errmsg(db));
	} else {
		status = "Opened database successfully";
	}
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), status.c_str());
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "database", aBuf);

	std::string sql = " BEGIN; \
				CREATE TABLE IF NOT EXISTS PLAYERS( \
					name TEXT PRIMARY KEY, \
					totalOfPlayedRounds INTEGER DEFAULT 0, \
					totalOfKills INTEGER DEFAULT 0, \
					totalOfKillsAsZombie INTEGER DEFAULT 0, \
					totalOfTimeInGame INTEGER DEFAULT 0, \
					totalOfWinsAsHuman INTEGER DEFAULT 0, \
					totalOfScore INTEGER DEFAULT 0, \
					topScore INTEGER DEFAULT 0, \
					topNegativeScore INTEGER DEFAULT 0, \
					totalOfPlayerDeath INTEGER DEFAULT 0, \
					totalOfDistance INTEGER DEFAULT 0 \
				); \
				COMMIT;";
						
	rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
		
	/* check for error */
	if (rc != SQLITE_OK) {
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "SQL error (#%d): %s\n", rc, zErrMsg);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "database", aBuf);
		sqlite3_free(zErrMsg);
	}
	
	NewRound();
}

CDatabase::~CDatabase()
{
	m_runControlThread=false;
	if(m_ControlThread->joinable())
	{
		m_ControlThread->join();
		delete m_ControlThread;
	}
	sqlite3_close(db);
}


void CDatabase::NewRound()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{	
		m_aPlayers[i].id = -1;
	}
}

std::vector<std::string> CDatabase::GetStats(){
		
	std::vector<std::string> Tmp = vStats;
	vStats.clear();
	
	return Tmp;
}

void CDatabase::RoundStats()
{ 	
	bool TmpFirstLoop = true;
	int TmpBestKiller = 0;
	int TmpBestZombie = 0;
	int TmpBestRunner = 0;
	
	for (int i = 0; i< MAX_CLIENTS; i++){
		
		if(m_aPlayers[i].id != -1){
			
			if(TmpFirstLoop){
				TmpBestKiller = m_aPlayers[i].id;
				TmpBestZombie = m_aPlayers[i].id;
				TmpBestRunner = m_aPlayers[i].id;

				TmpFirstLoop = false;
			}
			
			if(m_aPlayers[i].Kills > m_aPlayers[TmpBestKiller].Kills){
				TmpBestKiller = i;
			}
			
			if(m_aPlayers[i].KillsAsZombie > m_aPlayers[TmpBestZombie].KillsAsZombie){
				TmpBestZombie = i;
			}
			
			if(m_aPlayers[i].Distance > m_aPlayers[TmpBestRunner].Distance){
				TmpBestRunner = i;
			}
		}
	}
	std::string Tmp;
	
	Tmp = "Top killer: " + ReplaceAll(m_aPlayers[TmpBestKiller].Name, std::string("''"), std::string("'")) + " with " + std::to_string(m_aPlayers[TmpBestKiller].Kills) + " kills (ping " + std::to_string(m_aPlayers[TmpBestKiller].AvgPing) + ")";
	vStats.push_back(Tmp);
	Tmp = "Top zombie: " + ReplaceAll(m_aPlayers[TmpBestZombie].Name, std::string("''"), std::string("'")) + " with " + std::to_string(m_aPlayers[TmpBestZombie].KillsAsZombie) + " infects (ping " + std::to_string(m_aPlayers[TmpBestZombie].AvgPing) + ")";
	vStats.push_back(Tmp);
	Tmp = "Top runner: " + ReplaceAll(m_aPlayers[TmpBestRunner].Name, std::string("''"), std::string("'")) + " with " + std::to_string((int)m_aPlayers[TmpBestRunner].Distance / m_aPlayers[TmpBestRunner].TimeInGame)+ " teexels/s (ping " + std::to_string(m_aPlayers[TmpBestKiller].AvgPing) + ")";
	vStats.push_back(Tmp);
	
}

void CDatabase::AddRoundStats(int id, std::string Name, int Kills, int KillsAsZombie, int AvgPing, int TimeInGame, bool WonAsHuman, int Score, int Death, float Distance)
{
	m_aPlayers[id] = {id, Name, Kills, KillsAsZombie, AvgPing, TimeInGame, WonAsHuman, Score, Death, Distance};	

	std::string tName = ReplaceAll(Name, std::string("'"), std::string("''")); //sql iject protection
	m_RankingThreads.push_back( new std::thread( [=] {taskAddRoundStats(id, tName, Kills, KillsAsZombie, AvgPing, TimeInGame, WonAsHuman, Score, Death, Distance);} ) );
}

void CDatabase::PlayerStats(std::string RequestedBy, std::string Name)
{	
	std::string tName = ReplaceAll(Name, std::string("'"), std::string("''"));
	m_RankingThreads.push_back( new std::thread([=] { taskPlayerStats( RequestedBy, tName);} ) );
}

void CDatabase::Top5(std::string RequestedBy, std::string Name)
{	
	std::string tName = ReplaceAll(Name, std::string("'"), std::string("''"));
	m_RankingThreads.push_back( new std::thread([=] { taskTop5( RequestedBy, tName);} ) );
}

//threads
void CDatabase::taskPlayerStats(std::string RequestedBy, std::string Name)
{
	m_mutexDatabaseFree.lock();
	
    vStats.push_back("Stats for: " + ReplaceAll(Name, std::string("''"), std::string("'"))  + " requested by (" + RequestedBy +")");	
	
	char *zErrMsg = 0;
	
	std::string sql = "select *, (select count(*) from players pl1  where pl1.totalOfWinsAsHuman > pl2.totalOfWinsAsHuman)+1 as rank from Players pl2 where name == '"+Name+"'";
	rc = sqlite3_exec(db, sql.c_str(), callbackRank, (void*) this, &zErrMsg);

	// check for error 
	if (rc != SQLITE_OK)
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "SQL error (#%d): %s\n", rc, zErrMsg);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "database", aBuf);
		sqlite3_free(zErrMsg);
	}
	m_mutexDatabaseFree.unlock();
}

void CDatabase::taskAddRoundStats(int id, std::string Name, int Kills, int KillsAsZombie, int AvgPing, int TimeInGame, bool WonAsHuman, int Score, int Death, float Distance)
{
	m_mutexDatabaseFree.lock();	

	char *zErrMsg = 0;
	
			
	std::string sql = 	"INSERT OR IGNORE INTO PLAYERS  (name, totalOfPlayedRounds, totalOfKills, totalOfKillsAsZombie, totalOfTimeInGame, totalOfWinsAsHuman, totalOfScore, topScore, topNegativeScore, totalOfPlayerDeath, totalOfDistance) \
						VALUES ('"+Name+"', 0, 0, 0, 1, 0, 0, 0, 0, 0, 0); \
						UPDATE PLAYERS SET totalOfPlayedRounds = totalOfPlayedRounds + 1 WHERE name = '"+Name+"'; \
						UPDATE PLAYERS SET totalOfKills = totalOfKills + "+std::to_string(Kills)+" WHERE name = '"+Name+"'; \
						UPDATE PLAYERS SET totalOfKillsAsZombie = totalOfKillsAsZombie + "+std::to_string(KillsAsZombie)+" WHERE name = '"+Name+"'; \
						UPDATE PLAYERS SET totalOfTimeInGame = totalOfTimeInGame + "+std::to_string(TimeInGame)+" WHERE name = '"+Name+"'; \
						UPDATE PLAYERS SET totalOfWinsAsHuman = totalOfWinsAsHuman + "+ std::to_string(WonAsHuman ? 1 : 0) +" WHERE name = '"+Name+"'; \
						UPDATE PLAYERS SET totalOfScore = totalOfScore + "+ std::to_string(Score) +" WHERE name = '"+Name+"'; \
						UPDATE PLAYERS SET topScore = '"+std::to_string(Score)+"' WHERE topScore < '"+std::to_string(Score)+"' AND name = '"+Name+"'; \
						UPDATE PLAYERS SET topNegativeScore = '"+std::to_string(Score)+"' WHERE topNegativeScore > '"+std::to_string(Score)+"' AND name = '"+Name+"'; \
						UPDATE PLAYERS SET totalOfPlayerDeath = totalOfPlayerDeath + "+std::to_string(Death)+" WHERE name = '"+Name+"'; \
						UPDATE PLAYERS SET totalOfDistance = totalOfDistance + "+std::to_string((int)Distance)+" WHERE name = '"+Name+"'; ";

	rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
 
	// check for error 
	if (rc != SQLITE_OK) {
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "SQL error (#%d): %s\n", rc, zErrMsg);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "database", aBuf);
		sqlite3_free(zErrMsg);
	}
	
	m_mutexDatabaseFree.unlock();
}

void CDatabase::taskTop5(std::string RequestedBy, std::string Name)
{
	m_mutexDatabaseFree.lock();
		
    vStats.push_back("Top 5 " + ReplaceAll(Name, std::string("''"), std::string("'"))  + " requested by (" + RequestedBy +")");	
	
	char *zErrMsg = 0;
	
	std::string sql = "";
	for(int i = 0; i< 5; i++){
		if(Name == "runners"){		
			sql = "SELECT Name as runners, totalOfDistance as distance, totalOfTimeInGame as time FROM Players ORDER BY totalOfDistance/totalOfTimeInGame desc limit 1 offset "+std::to_string(i)+"";
		}
		if(Name == "killers"){		
			sql = "SELECT Name as killers, totalOfKills as kills, totalOfPlayedRounds as rounds FROM Players ORDER BY totalOfKills desc, totalOfPlayedRounds limit 1 offset "+std::to_string(i)+"";
		}
		if(Name == "zombies"){		
			sql = "SELECT Name as zombies, totalOfKillsAsZombie as kills, totalOfPlayedRounds as rounds FROM Players ORDER BY totalOfKillsAsZombie desc, totalOfPlayedRounds limit 1 offset "+std::to_string(i)+"";
		}
		rc = sqlite3_exec(db, sql.c_str(), callbackTop5, (void*) this, &zErrMsg);

		// check for error 
		if (rc != SQLITE_OK)
		{
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "SQL error (#%d): %s\n", rc, zErrMsg);
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "database", aBuf);
			sqlite3_free(zErrMsg);
		}
	}
	m_mutexDatabaseFree.unlock();
}


int CDatabase::callbackTop5(void *pointerCDatabase, int count, char **data, char **columns)
{
	CDatabase* database = (CDatabase*) pointerCDatabase;
	int Kills = 0; 
	int PlayedRounds = 0;
	int TotalDistance = 0;
	std::string Name;
	int Time = 0;
	std::string Tmp;	

	for (int idx = 0; idx < count; idx++)
	{
		if(	std::string(columns[idx]) == "runners" || std::string(columns[idx]) == "killers" || std::string(columns[idx]) == "zombies")
		{
			Tmp = std::string(columns[idx]);
			Name = std::string(data[idx]) ;
		}

		if(	std::string(columns[idx]) == "distance" )
			TotalDistance = std::stoi( std::string(data[idx]) );	
		if(	std::string(columns[idx]) == "time" )
			Time = std::stoi( std::string(data[idx]) );	
		if(	std::string(columns[idx]) == "kills")
			Kills = std::stoi( std::string(data[idx]) );
		if(	std::string(columns[idx]) == "rounds" )
			PlayedRounds = std::stoi( std::string(data[idx]) );
		
	}
	
	
	if (Tmp == "killers")
	{
		database->vStats.push_back(ReplaceAll(Name, std::string("''"), std::string("'")) + "   " + std::to_string( Kills / PlayedRounds )+ "   avg. kills");
	}
	if (Tmp == "runners")
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << (float) TotalDistance / (float) (Time == 0 ? 1 : Time);
		database->vStats.push_back(ReplaceAll(Name, std::string("''"), std::string("'")) + "   " + stream.str() + "   teexels/s" );
	}
	if(Tmp == "zombies")
	{
		database->vStats.push_back(ReplaceAll(Name, std::string("''"), std::string("'")) + "   " + std::to_string( Kills / PlayedRounds ) + "   avg. infects" );	
	}
	
	return 0;
}

int CDatabase::callbackRank(void *pointerCDatabase, int count, char **data, char **columns)
{
	CDatabase* database = (CDatabase*) pointerCDatabase;
	int Kills = 0; 
	int Death = 0;
	int PlayedRounds = 0;
	int TotalTimeInGame = 0;
	int TotalDistance = 0;
	int Rank = -1;
	int Wins = 0;
	int topScore = 0;
	
	for (int idx = 0; idx < count; idx++)
	{
		if(	std::string(columns[idx]) == "totalOfKills" )
			Kills = std::stoi( std::string(data[idx]) );
		if(	std::string(columns[idx]) == "totalOfPlayerDeath" )
			Death = std::stoi( std::string(data[idx]) );
		if(	std::string(columns[idx]) == "totalOfPlayedRounds" )
			PlayedRounds = std::stoi( std::string(data[idx]) );	   
		if(	std::string(columns[idx]) == "totalOfTimeInGame" )
			TotalTimeInGame = std::stoi( std::string(data[idx]) );
		if(	std::string(columns[idx]) == "totalOfDistance" )
			TotalDistance = std::stoi( std::string(data[idx]) );
		if(	std::string(columns[idx]) == "rank" )
			Rank = std::stoi( std::string(data[idx]) );
		if(	std::string(columns[idx]) == "totalOfWinsAsHuman" )
			Wins = std::stoi( std::string(data[idx]) );
		if(	std::string(columns[idx]) == "topScore" )
			topScore = std::stoi( std::string(data[idx]) );
	}
	
	if(Wins == 0)
		Rank = -1;
	std::stringstream stream;
	stream << std::fixed << std::setprecision(2) << (float) Kills / (float) (Death == 0 ? 1 : Death);
	
	if(Rank != -1)
		database->vStats.push_back(std::to_string(Rank) + ". place");
	else
		database->vStats.push_back("Has not won yet :(");
	database->vStats.push_back("Kill/Death rate: " +  stream.str() );
	database->vStats.push_back("Average kills per round : " + std::to_string( Kills / PlayedRounds ) );
	database->vStats.push_back("Average death per round : " + std::to_string( Death / PlayedRounds ) );
	database->vStats.push_back("Average speed per round : " + std::to_string( TotalDistance / TotalTimeInGame ) );
	database->vStats.push_back("Top score : " + std::to_string( topScore ) );
	
	return 0;
}

std::string CDatabase::ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}