#ifndef GAME_SERVER_BOT_H
#define GAME_SERVER_BOT_H

#include <base/vmath.h>
#include <engine/shared/config.h>

#include "gamecontext.h"
#include "botengine.h"

#include "ai/genetics.h"

const char g_BotClan[12] = {*g_Config.m_SvBotClan};
const char g_BotName[MAX_CLIENTS][16] = {
	"Anna",
	"Bob",
	"Carlos",
	"David",
	"Eli",
	"Florianne",
	"Gaia",
	"Hannibal",
	"Isis",
	"Juda",
	"Kevin",
	"Lucile",
	"Marc",
	"Naustradamus",
	"Ondine",
	"Platon"
};
extern char g_BotSkin[MAX_CLIENTS][16]; //made this one extern to avoid a segmentation fault and weird compilation -skay

#define	BOT_HOOK_DIRS	32

#define BOT_CHECK_TIME (20*60*1000000)

class CBot
{
	class CBotEngine *m_pBotEngine;
	class CPlayer *m_pPlayer;
	class CGameContext *m_pGameServer;

protected:

	class CBotEngine *BotEngine() { return m_pBotEngine; }
	class CGameContext *GameServer() { return BotEngine()->GameServer(); }

	class CCollision *Collision() { return GameServer()->Collision(); }
	class CTuningParams *Tuning() { return GameServer()->Tuning(); }

	CBotEngine::CPath *m_pPath;

	int m_SnapID;

	enum {
		BFLAG_LOST	= 0,
		BFLAG_LEFT	= 1,
		BFLAG_RIGHT	= 2,
		BFLAG_JUMP	= 4,
		BFLAG_HOOK	= 8,
		BFLAG_FIRE	= 16
	};

	int m_Flags;

	vec2 m_Target;
	vec2 m_RealTarget;
	struct CTarget {
		vec2 m_Pos;
		enum {
			TARGET_EMPTY=-1,
			TARGET_PLAYER=0,
			TARGET_FLAG,
			TARGET_ARMOR,
			TARGET_HEALTH,
			TARGET_WEAPON_SHOTGUN,
			TARGET_WEAPON_GRENADE,
			//TARGET_POWERUP_NINJA,
			TARGET_WEAPON_LASER,
			TARGET_AIR,
			NUM_TARGETS
		};
		int m_Type;
		int m_PlayerCID;
		bool m_NeedUpdate;
		int m_StartTick;
	} m_ComputeTarget;

	class CGenetics m_Genetics;
	int m_aTargetOrder[CTarget::NUM_TARGETS];

	void UpdateTargetOrder();

	CNetObj_PlayerInput m_InputData;
	CNetObj_PlayerInput m_LastData;

	int m_Move;
	vec2 m_Direction;
	int m_Jump;
	int m_Attack;
	int m_Hook;

	int GetTarget();
	void UpdateTarget();
	int GetTeam(int ClientID);
	int IsFalling();
	bool IsGrounded();

	bool NeedPickup(int Type);
	bool FindPickup(int Type, vec2 *pPos, float Radius = 1000);

	void HandleWeapon(bool SeeTarget);
	void HandleHook(bool SeeTarget);
	void UpdateEdge();
	void MakeChoice(bool UseTarget);

	int GetTile(int x, int y) { return BotEngine()->GetTile(x/32,y/32);}

	vec2 ClosestCharacter();

	bool sameTeam(int player);
	bool sameInfTeam(int player);

public:
	CBot(class CBotEngine *m_pBotEngine, CPlayer *pPlayer);
	virtual ~CBot();


	int m_GenomeTick;

	const char *GetName();
	const char *GetSkin();
	const char *GetClan() { return g_BotClan; }

	int GetID() { return m_SnapID; }
	void Snap(int SnappingClient);
	void Tick();

	virtual void OnReset();

	CNetObj_PlayerInput GetInputData() { Tick(); return m_InputData; };
	CNetObj_PlayerInput GetLastInputData() { return m_LastData; }

	const char *GetRepartee();

	void OnChatMessage(int SenderClientId);
	void SayTo(int ClientId, const char* pMessage);
	void Say(const char* pMessage, int team = CGameContext::CHAT_ALL);
	void Whisper(int ClientId, const char* pMessage);

};

#endif
