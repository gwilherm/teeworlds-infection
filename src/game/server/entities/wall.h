#ifndef GAME_SERVER_ENTITIES_WALL_H
#define GAME_SERVER_ENTITIES_WALL_H

#include <game/server/entity.h>

class CWall: public CEntity {
public:
    
    CWall(CGameWorld *pWorld, vec2 From, vec2 To, int Owner);
    
    virtual void Reset();
    virtual void Tick();
    virtual void TickPaused();
    virtual void Snap(int SnappingClient);
    
    bool HitCharacter(vec2 From, vec2 To, class CCharacter *pCharacter);
    
    int m_Owner;
    int m_StartTick;
    bool m_Active;
    vec2 m_To;
};

#endif // GAME_SERVER_ENTITIES_WALL_H
