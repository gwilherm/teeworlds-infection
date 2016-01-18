#include <game/server/gameworld.h>
#include "wall.h"
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

CWall::CWall(CGameWorld *pWorld, vec2 From, vec2 To, int Owner):
    CEntity(pWorld, CGameWorld::ENTTYPE_WALL) {
    m_Owner = Owner;
    m_StartTick = Server()->Tick();
    m_Pos = From;
    m_To = To;
    if (distance(To, From) >= g_Config.m_InfWallLength) {
        float Angle = atan2(To.y - From.y, To.x - From.x);
        m_To = vec2(cos(Angle) * g_Config.m_InfWallLength,
                    sin(Angle) * g_Config.m_InfWallLength) + From;
    }
    pWorld->InsertEntity(this);
}

bool CWall::HitCharacter(vec2 From, vec2 To, CCharacter *pCharacter) {
    vec2 IntersectPos;

    if(distance(From, To) > 0)
    {
		IntersectPos = closest_point_on_line(From, To, pCharacter->m_Pos);
	}
	else
	{
		IntersectPos = From;
	}

    return distance(pCharacter->m_Pos, IntersectPos) < pCharacter->m_ProximityRadius;
}

void CWall::Tick() {
    m_Active = true;
    if (!GameServer()->m_apPlayers[m_Owner]) {
        Reset();
        return;
    } else if (GameServer()->m_apPlayers[m_Owner]->GetTeam() == TEAM_SPECTATORS ||
               GameServer()->m_apPlayers[m_Owner]->Infected() ||
               Server()->Tick() >= m_StartTick + Server()->TickSpeed() * g_Config.m_InfWallLife) {
        Reset();
        return;
    } else if (Server()->Tick() < m_StartTick + Server()->TickSpeed() * g_Config.m_InfWallDelay) {
        m_Active = false;
        return;
    }

    CCharacter *pCharacter = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER);
    while (pCharacter) {
        if (!pCharacter)
            return;

        if (pCharacter->GetPlayer()->Infected())
            if (HitCharacter(m_Pos, m_To, pCharacter))
                pCharacter->Die(m_Owner, WEAPON_WORLD);

        pCharacter = (CCharacter *)pCharacter->TypeNext();
    }
}

void CWall::Reset() {
    GameWorld()->DestroyEntity(this);
    if(GameServer()->GetPlayerChar(m_Owner))
	{
        GameServer()->GetPlayerChar(m_Owner)->m_pWall = 0;
        GameServer()->GetPlayerChar(m_Owner)->m_WallStart = vec2(0,0);
	}
}

void CWall::TickPaused() {
    m_StartTick ++;
}

void CWall::Snap(int SnappingClient) {
    if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

    if (m_Active) {
        pObj->m_X = (int)m_To.x;
        pObj->m_Y = (int)m_To.y;
    } else {
        pObj->m_X = (int)m_Pos.x;
        pObj->m_Y = (int)m_Pos.y;
    }
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = Server()->Tick();
}
