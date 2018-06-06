#include "infection.h"

#include <cstdio>

#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>

#include <game/server/bot.h>

CGameControllerInfection::CGameControllerInfection(class CGameContext *pGameServer)
: IGameController(pGameServer) {
    m_pGameType = "bInfection+";
	m_BroadcastTime = 0;
	m_NextIdToPick = 0;
}

void CGameControllerInfection::DoWincheck() {
    int Humans = 0, Zombies = 0;
    for (int i = 0; i < MAX_CLIENTS; i ++) {
        CPlayer *pPlayer = GameServer()->m_apPlayers[i];
        if (!pPlayer)
            continue;

        if (pPlayer->GetTeam() == TEAM_SPECTATORS)
            continue;

        if (pPlayer->Infected())
            Zombies ++;
        else
            Humans ++;
    }

    if (Humans + Zombies < 2) {
        m_SuddenDeath = true;
        m_Warmup = 0;
        GameServer()->SendBroadcast("At least 2 players required to start playing", -1);
        return;
    } else if (m_SuddenDeath) {
        StartRound();
        return;
    }

    if (m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested) {
        if (!Humans || !Zombies) {
            EndRound();
            return;
        }
    }
    IGameController::DoWincheck();
}

void CGameControllerInfection::OnCharacterSpawn(CCharacter *pChr) {
    pChr->IncreaseHealth(10);
    pChr->GiveWeapon(WEAPON_HAMMER, -1);
    if (pChr->GetPlayer()->Infected())
        pChr->SetWeapon(WEAPON_HAMMER);
    else
        pChr->GiveWeapon(WEAPON_GUN, 10);
}

int CGameControllerInfection::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon) {

	if (!pKiller || Weapon == WEAPON_GAME)
		return 0;

	if (pKiller == pVictim->GetPlayer()){
		pVictim->GetPlayer()->m_Score--; // suicide
		pVictim->GetPlayer()->m_Statistics.m_RoundScore--;
	}
	else {
		if((IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam()) ||
            IsFriendlyFire(pVictim->GetPlayer()->GetCID(), pKiller->GetCID()))
        {
            // teamkill
            pKiller->m_Score -= g_Config.m_SvTkPenalty;
			pKiller->m_Statistics.m_RoundScore -=g_Config.m_SvTkPenalty;
        }
		else
        {
            // normal kill
            pKiller->m_Score++;
			pKiller->m_Statistics.m_RoundScore++;
        }
		if (Weapon == WEAPON_GUN || Weapon == WEAPON_HAMMER || Weapon == WEAPON_SHOTGUN || Weapon == WEAPON_GRENADE || Weapon == WEAPON_RIFLE)
			pKiller->m_Statistics.m_RoundKillsWeapons++;
		else if(Weapon == WEAPON_WORLD)
			pKiller->m_Statistics.m_RoundKillsBariers++;

		if(pKiller->Infected())
			pKiller->m_Statistics.m_RoundKillsAsZombie++;
		pVictim->GetPlayer()->m_Statistics.m_RoundDeath++;
	}

	if (Weapon == WEAPON_SELF)
    {
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;
    }
    else
    {
        if(!pKiller->Infected() || !pKiller->m_HasSuperJump)
            pKiller->m_Kills++;

        if (pKiller->Infected()) {
            if (pKiller->m_Kills >= g_Config.m_InfSuperJumpKills && !pKiller->m_HasSuperJump) {
                pKiller->m_Kills = 0;
                pKiller->m_HasSuperJump = true;
                GameServer()->SendBroadcast("You earned super jump, hold spacebar to use it", pKiller->GetCID());
                char str[512] = {0};
                if(pKiller->IsBot() && pKiller->m_pBot)
                    sprintf(str, g_Config.m_InfSuperJumpText, pKiller->m_pBot->GetName());
                else
                    sprintf(str, g_Config.m_InfSuperJumpText, Server()->ClientName(pKiller->GetCID()));
                GameServer()->SendChatTarget(-1, str);
            }
        } else {
		if (pKiller->m_AirstrikeNotFirework || g_Config.m_InfFireworkKills == 0) {
			if (pKiller->m_Kills >= g_Config.m_InfAirstrikeKills && !pKiller->m_HasAirstrike) {
				if (g_Config.m_InfAirstrikeKills != 0){
					pKiller->m_Kills -= g_Config.m_InfAirstrikeKills;
					pKiller->m_HasAirstrike = true;
					GameServer()->SendBroadcast("You got an airstrike, use hammer to launch it :D", pKiller->GetCID());
					char str[512] = {0};
					if(pKiller->IsBot() && pKiller->m_pBot)
						sprintf(str, g_Config.m_InfAirstrikeText, pKiller->m_pBot->GetName());
					else
						sprintf(str, g_Config.m_InfAirstrikeText, Server()->ClientName(pKiller->GetCID()));
					GameServer()->SendChatTarget(-1, str);
				}
				
				pKiller->m_AirstrikeNotFirework = false;
			}
		} else {
			if (pKiller->m_Kills >= g_Config.m_InfFireworkKills && !pKiller->m_HasFirework) {
				if (g_Config.m_InfFireworkKills != 0){
					pKiller->m_Kills -= g_Config.m_InfFireworkKills;
					pKiller->m_HasFirework = true;
					GameServer()->SendBroadcast("You got a firework, use hammer to launch it ^^", pKiller->GetCID());
					char str[512] = {0};
					if(pKiller->IsBot() && pKiller->m_pBot)
						sprintf(str, g_Config.m_InfFireworkText, pKiller->m_pBot->GetName());
					else
						sprintf(str, g_Config.m_InfFireworkText, Server()->ClientName(pKiller->GetCID()));
					GameServer()->SendChatTarget(-1, str);
				}
				
				pKiller->m_AirstrikeNotFirework = true;
			}
		}
        }
    }
	return 0;
}

void CGameControllerInfection::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_Kills = 0;
			GameServer()->m_apPlayers[i]->m_HasSuperJump = false;
			GameServer()->m_apPlayers[i]->m_HasAirstrike = false;
			GameServer()->m_apPlayers[i]->m_HasFirework = false;
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
		}
	}
}
