/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "gamecontext.h"

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Init(int CID);

	void TryRespawn();
	void Respawn();
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; }
	int GetCID() const { return m_ClientID; }

    void Infect(int By = -1, int Weapon = WEAPON_HAMMER);
    void Cure(int By = -1, int WEAPON = WEAPON_GAME);
    inline bool Infected() { return (m_Zombie != HUMAN); }

    bool SpawnProtection();

	bool IsInfTeam(int ClientID);

    void SetCID(int ClientID);

	void Mute() { m_Muted = true; }
	void Unmute() { m_Muted = false; }
    bool IsMuted() { return m_Muted; }

	void Tick();
	void PostTick();
	void Snap(int SnappingClient);

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;

	bool m_IsReady;

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	//stats
	struct
	{
		int m_RoundKills;
		int m_RoundKillsAsZombie;
		int m_RoundTimeInGame;
		bool m_WonRoundAsHuman;
		int m_RoundScore;
		int m_RoundDeath;
		float m_Distance;
		vec2 m_OldPos;
		vec2 m_Pos;
	} m_Statistics;
	
	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_RespawnTick;
	int m_DieTick;
	int m_Score;
	int m_ScoreStartTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

    enum
    {
		HUMAN = 0,
		ZOMBIE = 1,
		I_ZOMBIE = 2
    };
    int m_Zombie;
    int m_Kills;
	bool m_AirstrikeNotFirework;
    bool m_HasSuperJump;
    bool m_HasAirstrike;
	bool m_HasFirework;

    int m_LastEyeEmote;
    int m_DefEmote;
    int m_DefEmoteReset;

private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	//
	bool m_Spawning;
	int m_ClientID;
	int m_Team;
	bool m_Muted;
};

#endif
