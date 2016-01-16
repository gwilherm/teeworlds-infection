#ifndef GAME_SERVER_GAMEMODES_INFECTION_H
#define GAME_SERVER_GAMEMODES_INFECTION_H

#include <game/server/gamecontroller.h>

class CGameControllerInfection : public IGameController
{
public:
	CGameControllerInfection(class CGameContext *pGameServer);
	
	virtual void DoWincheck();
	
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	
	virtual void PostReset();
	
	int m_BroadcastTime;
};

#endif // GAME_SERVER_GAMEMODES_INFECTION_H
