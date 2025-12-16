//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CS's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "cs_player.h"
#include "player_resource.h"
#include "cs_simple_hostage.h"
#include "cs_player_resource.h"
#include "weapon_c4.h"
#include <coordsize.h>
#include "cs_bot_manager.h"
#include "cs_bot.h"
#include "cs_gamerules.h"

extern ConVar mp_teammates_are_enemies;

// Datatable
IMPLEMENT_SERVERCLASS_ST(CCSPlayerResource, DT_CSPlayerResource)
	SendPropInt( SENDINFO( m_iPlayerC4 ), 8, SPROP_UNSIGNED ),
	SendPropVector( SENDINFO(m_vecC4), -1, SPROP_COORD),
	SendPropArray3( SENDINFO_ARRAY3(m_bHostageAlive), SendPropInt( SENDINFO_ARRAY(m_bHostageAlive), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_isHostageFollowingSomeone), SendPropInt( SENDINFO_ARRAY(m_isHostageFollowingSomeone), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iHostageEntityIDs), SendPropInt( SENDINFO_ARRAY(m_iHostageEntityIDs), -1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iHostageY), SendPropInt( SENDINFO_ARRAY(m_iHostageY), COORD_INTEGER_BITS+1, 0 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iHostageX), SendPropInt( SENDINFO_ARRAY(m_iHostageX), COORD_INTEGER_BITS+1, 0 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iHostageZ), SendPropInt( SENDINFO_ARRAY(m_iHostageZ), COORD_INTEGER_BITS+1, 0 ) ),
	SendPropVector( SENDINFO(m_bombsiteCenterA), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_bombsiteCenterB), -1, SPROP_COORD),
	SendPropArray3( SENDINFO_ARRAY3(m_hostageRescueX), SendPropInt( SENDINFO_ARRAY(m_hostageRescueX), COORD_INTEGER_BITS+1, 0 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_hostageRescueY), SendPropInt( SENDINFO_ARRAY(m_hostageRescueY), COORD_INTEGER_BITS+1, 0 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_hostageRescueZ), SendPropInt( SENDINFO_ARRAY(m_hostageRescueZ), COORD_INTEGER_BITS+1, 0 ) ),
	SendPropBool( SENDINFO( m_bBombSpotted ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bPlayerSpotted), SendPropInt( SENDINFO_ARRAY(m_bPlayerSpotted), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bHostageSpotted), SendPropInt( SENDINFO_ARRAY(m_bHostageSpotted), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iMVPs), SendPropInt( SENDINFO_ARRAY(m_iMVPs), COORD_INTEGER_BITS+1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bHasDefuser), SendPropInt( SENDINFO_ARRAY(m_bHasDefuser), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iAccount), SendPropInt( SENDINFO_ARRAY(m_iAccount), COORD_INTEGER_BITS+1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iGunGameProgressiveWeaponIndex), SendPropInt( SENDINFO_ARRAY(m_iGunGameProgressiveWeaponIndex), COORD_INTEGER_BITS+1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iContributionScore), SendPropInt( SENDINFO_ARRAY(m_iContributionScore), 32) ),
	SendPropArray3( SENDINFO_ARRAY3(m_nMusicID), SendPropInt( SENDINFO_ARRAY(m_nMusicID), 32) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iCompTeammateColor ), SendPropInt( SENDINFO_ARRAY( m_iCompTeammateColor ), 32 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bControllingBot), SendPropInt( SENDINFO_ARRAY(m_bControllingBot), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iControlledPlayer), SendPropInt( SENDINFO_ARRAY(m_iControlledPlayer), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iControlledByPlayer), SendPropInt( SENDINFO_ARRAY(m_iControlledByPlayer), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_szClan), SendPropStringT( SENDINFO_ARRAY(m_szClan) ) ),
END_SEND_TABLE()
//=============================================================================
// HPE_END
//=============================================================================

BEGIN_DATADESC( CCSPlayerResource )
	// DEFINE_ARRAY( m_iPing, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( m_iPacketloss, FIELD_INTEGER, MAX_PLAYERS+1 ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( cs_player_manager, CCSPlayerResource );

CCSPlayerResource::CCSPlayerResource( void )
{
	m_bPreferencesAssigned_T = false;
	m_bPreferencesAssigned_CT = false;
	memset( m_nAttemptedToGetColor, false, sizeof( m_nAttemptedToGetColor ) );
}


//--------------------------------------------------------------------------------------------------------
class Spotter
{
public:
	Spotter( CBaseEntity *entity, const Vector &target, int spottingTeam )
	{
		m_targetEntity = entity;
		m_target = target;
		m_team = spottingTeam;
		m_spotted = false;
	}

	bool operator()( CBasePlayer *player )
	{
		if ( m_targetEntity->IsPlayer() )
		{
			if ( !mp_teammates_are_enemies.GetBool() && (!player->IsAlive() || player->GetTeamNumber() != m_team) )
				return true;
		}
		else
		{
			if ( !player->IsAlive() || player->GetTeamNumber() != m_team )
				return true;
		}

		CCSPlayer *csPlayer = ToCSPlayer( player );
		if ( !csPlayer )
			return true;

		if ( csPlayer->IsBlind() )
			return true;

		Vector eye, forward;
		player->EyePositionAndVectors( &eye, &forward, NULL, NULL );
		Vector path( m_target - eye );
		float distance = path.Length();
		path.NormalizeInPlace();
		float dot = DotProduct( forward, path );
		if( (dot > 0.995f ) 
			|| (dot > 0.98f && distance < 900) 
			|| (dot > 0.8f && distance < 250) 
			)
		{
			trace_t tr;
			CTraceFilterSkipTwoEntities filter( player, m_targetEntity, COLLISION_GROUP_DEBRIS );
			UTIL_TraceLine( eye, m_target,
				(CONTENTS_OPAQUE|CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_DEBRIS), &filter, &tr );

			if( tr.fraction == 1.0f )
			{
				if ( TheCSBots()->IsLineBlockedBySmoke( eye, m_target ) )
				{
					return true;
				}

				m_spotted = true;
				return false; // spotted already, so no reason to check for other players spotting the same thing.
			}
		}

		return true;
	}

	bool Spotted( void ) const
	{
		return m_spotted;
	}

private:
	CBaseEntity *m_targetEntity;
	Vector m_target;
	int m_team;
	bool m_spotted;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCSPlayerResource::UpdatePlayerData( void )
{
	int i;

	m_iPlayerC4 = 0;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CCSPlayer *pPlayer = (CCSPlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{

			if ( pPlayer->HasC4() )
			{
				// we should only have one bomb
				m_iPlayerC4 = i;
			}

			m_szClan.Set(i, AllocPooledString( pPlayer->GetClanTag() ) );
			m_iMVPs.Set(i, pPlayer->GetNumMVPs());
			m_bHasDefuser.Set(i, pPlayer->HasDefuser());
			m_iAccount.Set( i, pPlayer->GetAccountBalance() );
			m_iGunGameProgressiveWeaponIndex.Set( i, pPlayer->m_iGunGameProgressiveWeaponIndex );
			m_iContributionScore.Set( i, pPlayer->GetContributionScore() );
			m_nMusicID.Set( i, pPlayer->m_iLoadoutMusic );
			
			if ( pPlayer->IsBot() )
			{
				CCSBot* pBot = dynamic_cast< CCSBot* >( pPlayer );

				if ( pBot )
				{
					// Retrieve and store the bot's difficulty level
					const BotProfile* pProfile = pBot->GetProfile();

					if ( pProfile )
					{
						botDifficulty = pProfile->GetMaxDifficulty();
					}

					m_iCompTeammateColor.Set( i, -2 );
					m_nAttemptedToGetColor[i] = true;
//					SetPlayerTeammateColor( i, false );
				
				}
			}
			else
			{
				if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
					nTotalPlayingPlayers++;
			
				SetPlayerTeammateColor( i, false );
			}
		}
		else
		{
			m_szClan.Set( i, MAKE_STRING( "" ) );
			m_iMVPs.Set( i, 0 );
			m_iCompTeammateColor.Set( i, -1 );
			m_nAttemptedToGetColor[i] = false;
		}
	}

	CBaseEntity *c4 = NULL;
	if ( g_PlantedC4s.Count() > 0 )
	{
		c4 = g_PlantedC4s[0];
		m_vecC4 = c4->GetAbsOrigin();
	}
	else
	{
		if ( m_iPlayerC4 == 0 )
		{
			// no player has C4, update C4 position
			if ( g_C4s.Count() > 0 )
			{
				c4 = g_C4s[0];
				m_vecC4 = c4->GetAbsOrigin();
			}
			else
			{
				m_vecC4.Init();
			}
		}
	}

	int numHostages = g_Hostages.Count();

	for ( i = 0; i < MAX_HOSTAGES; i++ )
	{
		if ( i >= numHostages )
		{
//			engine->Con_NPrintf( i, "Dead" );
			m_bHostageAlive.Set( i, false );
			m_isHostageFollowingSomeone.Set( i, false );
			m_iHostageEntityIDs.Set( i, 0 );
			m_bHostageSpotted.Set( i, false );
			continue;
		}

		CHostage* pHostage = g_Hostages[i];

		m_bHostageAlive.Set( i, pHostage->IsRescuable() );

		if ( pHostage->IsValid() )
		{
			Spotter spotter( pHostage, pHostage->GetAbsOrigin(), TEAM_TERRORIST );
 			ForEachPlayer( spotter );
 			m_bHostageSpotted.Set( i, spotter.Spotted() );
			m_iHostageX.Set( i, (int) pHostage->GetAbsOrigin().x );	
			m_iHostageY.Set( i, (int) pHostage->GetAbsOrigin().y );	
			m_iHostageZ.Set( i, (int) pHostage->GetAbsOrigin().z );	
			m_iHostageEntityIDs.Set( i, pHostage->entindex() );
			m_isHostageFollowingSomeone.Set( i, pHostage->IsFollowingSomeone() );
//			engine->Con_NPrintf( i, "ID:%d Pos:(%.0f,%.0f,%.0f)", pHostage->entindex(), pHostage->GetAbsOrigin().x, pHostage->GetAbsOrigin().y, pHostage->GetAbsOrigin().z );
		}
		else
		{
//			engine->Con_NPrintf( i, "Invalid" );
		}
	}

	if( !m_foundGoalPositions )
	{
		// We only need to update these once a map, but we need the client to know about them.
		CBaseEntity* ent = NULL;
		while ( ( ent = gEntList.FindEntityByClassname( ent, "func_bomb_target" ) ) != NULL )
		{
			const Vector &pos = ent->WorldSpaceCenter();
			CNavArea *area = TheNavMesh->GetNearestNavArea( pos, true, 10000.0f, false, false );
			const char *placeName = (area) ? TheNavMesh->PlaceToName( area->GetPlace() ) : NULL;
			if ( placeName == NULL )
			{
				// The bomb site has no area or place name, so just choose A then B
				if ( m_bombsiteCenterA.Get().IsZero() )
				{
					m_bombsiteCenterA = pos;
				}
				else
				{
					m_bombsiteCenterB = pos;
				}
			}
			else
			{
				// The bomb site has a place name, so choose accordingly
				if( FStrEq( placeName, "BombsiteA" ) )
				{
					m_bombsiteCenterA = pos;
				}
				else
				{
					m_bombsiteCenterB = pos;
				}
			}
			m_foundGoalPositions = true;
		}

		int hostageRescue = 0;
		while ( (( ent = gEntList.FindEntityByClassname( ent, "func_hostage_rescue" ) ) != NULL)  &&  (hostageRescue < MAX_HOSTAGE_RESCUES) )
		{
			const Vector &pos = ent->WorldSpaceCenter();
			m_hostageRescueX.Set( hostageRescue, (int) pos.x );	
			m_hostageRescueY.Set( hostageRescue, (int) pos.y );	
			m_hostageRescueZ.Set( hostageRescue, (int) pos.z );	

			hostageRescue++;
			m_foundGoalPositions = true;
		}
	}

	bool bombSpotted = false;
	if ( c4 )
	{
		Spotter spotter( c4, m_vecC4, TEAM_CT );
		ForEachPlayer( spotter );
		if ( spotter.Spotted() )
		{
			bombSpotted = true;
		}
	}

	for ( int i=0; i < MAX_PLAYERS+1; i++ )
	{
		CCSPlayer *target = ToCSPlayer( UTIL_PlayerByIndex( i ) );

		if ( !target || !target->IsAlive() )
		{
			m_bPlayerSpotted.Set( i, 0 );
			continue;
		}

		Spotter spotter( target, target->EyePosition(), (target->GetTeamNumber()==TEAM_CT) ? TEAM_TERRORIST : TEAM_CT );
		ForEachPlayer( spotter );
		if ( spotter.Spotted() )
		{
			if ( target->HasC4() )
			{
				bombSpotted = true;
			}
			m_bPlayerSpotted.Set( i, 1 );
		}
		else
		{
			m_bPlayerSpotted.Set( i, 0 );
		}
	}

	if ( bombSpotted )
	{
		m_bBombSpotted = true;
	}
	else
	{
		m_bBombSpotted = false;
	}

	for ( int i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		CCSPlayer *pPlayer = ToCSPlayer( UTIL_PlayerByIndex( i ) );

		bool bControllingBot = false;
		CCSPlayer *pControlledPlayer = NULL;
		CCSPlayer *pControlledByPlayer = NULL;

		if ( pPlayer && pPlayer->IsConnected() )
		{
			bControllingBot = pPlayer->IsControllingBot();
			pControlledPlayer = pPlayer->GetControlledBot();
			pControlledByPlayer = pPlayer->GetControlledByPlayer();
		}

		m_bControllingBot.Set( i, bControllingBot ? 1 : 0 );
		m_iControlledPlayer.Set( i, pControlledPlayer ? pControlledPlayer->entindex() : 0 );
		m_iControlledByPlayer.Set( i, pControlledByPlayer ? pControlledByPlayer->entindex() : 0 );
	}

	BaseClass::UpdatePlayerData();
}

void CCSPlayerResource::Spawn( void )
{
	m_vecC4.Init();
	m_iPlayerC4 = 0;
	m_bombsiteCenterA.Init();
	m_bombsiteCenterB.Init();
	m_foundGoalPositions = false;
	memset( m_nAttemptedToGetColor, false, sizeof( m_nAttemptedToGetColor ) );

	for ( int i=0; i < MAX_HOSTAGES; i++ )
	{
		m_bHostageAlive.Set( i, 0 );
		m_isHostageFollowingSomeone.Set( i, 0 );
		m_iHostageEntityIDs.Set(i, 0);
		m_bHostageSpotted.Set(i, 0);
	}

	for ( int i=0; i < MAX_HOSTAGE_RESCUES; i++ )
	{
		m_hostageRescueX.Set( i, 0 );
		m_hostageRescueY.Set( i, 0 );
		m_hostageRescueZ.Set( i, 0 );
	}

	m_bBombSpotted = false;
	for ( int i=0; i < MAX_PLAYERS+1; i++ )
	{
		m_bPlayerSpotted.Set( i, 0 );
		m_szClan.Set( i, MAKE_STRING( "" ) );
		m_iMVPs.Set( i, 0 );
		m_bHasDefuser.Set(i, false);
		m_iAccount.Set( i, 0 );
		m_iGunGameProgressiveWeaponIndex.Set( i, 0 );
		m_iContributionScore.Set( i, 0 );
		m_nMusicID.Set( i, -1 );
		m_iCompTeammateColor.Set( i, -1 );
	}

	BaseClass::Spawn();
}

int CCSPlayerResource::GetCompTeammateColor( int iIndex )
{
	CCSPlayer *pPlayer = ( CCSPlayer* )UTIL_PlayerByIndex( iIndex );
	if ( !pPlayer )
		return -1;

	if ( pPlayer->IsBot() )
		return -2;

	return m_iCompTeammateColor[iIndex];
}

void CCSPlayerResource::ResetPlayerTeammateColor( int index )
{
	CCSPlayer *pPlayer = ( CCSPlayer* )UTIL_PlayerByIndex( index );
	if ( !pPlayer )
		return;
		
	if ( CSGameRules() && CSGameRules()->IsPlayingAnyCompetitiveStrictRuleset() )
		return;

	int nTeamNum = pPlayer->GetTeamNumber();
	if ( nTeamNum > TEAM_SPECTATOR )
	{
		SetPlayerTeammateColor( index, true );
		return;
	}

	m_iCompTeammateColor.Set( index, -1 );
}

void CCSPlayerResource::ForcePlayersPickColors()
{
	m_bPreferencesAssigned_CT = true;
	m_bPreferencesAssigned_T = true;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		m_nAttemptedToGetColor[i] = true;
}

void CCSPlayerResource::SetPlayerTeammateColor( int index, bool bReset )
{
	CCSPlayer *pPlayer = ( CCSPlayer* )UTIL_PlayerByIndex( index );
	if ( !pPlayer )
		return;

	m_nAttemptedToGetColor[index] = true;

	if ( !CSGameRules() || !CSGameRules()->IsPlayingAnyCompetitiveStrictRuleset() )
	{
		m_iCompTeammateColor.Set( index, -1 );
		return;
	}

 	if ( pPlayer->IsBot() )
 	{
 		m_iCompTeammateColor.Set( index, -2 );
 		return;
 	}

	int nTeamNum = pPlayer->GetTeamNumber();
	if ( nTeamNum > TEAM_SPECTATOR )
	{
		if ( CSGameRules() && CSGameRules()->IsPlayingAnyCompetitiveStrictRuleset() )
		{
			// check to see if we have a color already
			int idxThisPlayer = -1;

			// don't use the QMM code
			/*
			if ( CSGameRules()->IsQueuedMatchmaking() )
			{
				CCSPlayer *pThisPlayer = ( CCSPlayer* )UTIL_PlayerByIndex( index );
				CSteamID steamID;
				pThisPlayer->GetSteamID( &steamID );
				int numTotalPlayers = 0;		
				static ConVarRef sv_mmqueue_reservation( "sv_mmqueue_reservation" );
				for ( char const *pszPrev = sv_mmqueue_reservation.GetString(), *pszNext = pszPrev;
					  ( pszNext = strchr( pszPrev, '[' ) ) != NULL; pszPrev = pszNext + 1 )
				{
					uint32 uiAccountId = 0;
					sscanf( pszNext, "[%x]", &uiAccountId );
					if ( uiAccountId && ( steamID.GetAccountID() == uiAccountId ) )
					{
						idxThisPlayer = numTotalPlayers;
					}
					++numTotalPlayers;
				}
			}
			*/

			// let all players have at least one crack at getting their prefered color before we start assigning loser colors
			if ( (nTeamNum == TEAM_TERRORIST && m_bPreferencesAssigned_T == false) ||
				 (nTeamNum == TEAM_CT && m_bPreferencesAssigned_CT == false) )
			{
				int nNumAttemptedToGetColor = 0;
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CCSPlayer *pOtherPlayer = ( CCSPlayer* )UTIL_PlayerByIndex( i );
					if ( pOtherPlayer && pOtherPlayer->GetTeamNumber() == pPlayer->GetTeamNumber() )
					{
						if ( m_nAttemptedToGetColor[i] == true )
							nNumAttemptedToGetColor++;

						if ( nNumAttemptedToGetColor >= 5 )
						{
							if ( nTeamNum == TEAM_TERRORIST )
								m_bPreferencesAssigned_T = true;
							else
								m_bPreferencesAssigned_CT = true;

							break;
						}
					}
				}
			}

			// Valve MM gives us the player index - does this work?
			if ( idxThisPlayer > -1 )
			{
				m_iCompTeammateColor.Set( index, ( idxThisPlayer % 5 ) );
			}
			else if ( m_iCompTeammateColor[index] == -1 || bReset )//otherwise we have to do it ourselves
			{
				int nPreferredColor = pPlayer->GetTeammatePreferredColor( );
				if ( nPreferredColor == -1 )
				{
					pPlayer->InitTeammatePreferredColor( );
					nPreferredColor = pPlayer->GetTeammatePreferredColor( );
				}	

				// we didn't initialize, so try again another time
				if ( nPreferredColor == -1 )
					return;

				int nAssignedColor = m_iCompTeammateColor[index] > -1 ? m_iCompTeammateColor[index] : nPreferredColor;
				bool bColorInUse = false;
				for ( int ii = 0; ii < 5; ii++ )
				{
					nAssignedColor = nAssignedColor % 5;

					bColorInUse = false;
					for ( int j = 1; j <= gpGlobals->maxClients; j++ )
					{
						CCSPlayer *pOtherPlayer = ( CCSPlayer* )UTIL_PlayerByIndex( j );
						if ( pOtherPlayer && pOtherPlayer->GetTeamNumber( ) == pPlayer->GetTeamNumber( ) )
						{
							if ( nAssignedColor == m_iCompTeammateColor[j] && pOtherPlayer != pPlayer )
							{
								// All players should get a crack at getting their prefered color before a 
								// previously connected player crawls up the color scale and nabs it first
								if ( ( nTeamNum == TEAM_TERRORIST && m_bPreferencesAssigned_T == false) ||
									 ( nTeamNum == TEAM_CT && m_bPreferencesAssigned_CT == false ) )
									return;

								bColorInUse = true;
								nAssignedColor++;
								break;
							}
						}
					}

					if ( bColorInUse == false )
						break;
				}

				// somehow this failed
				AssertMsg( !bColorInUse, "Trying to assign a color to a teammate, but all colors are already in use!" );

				nAssignedColor = bColorInUse == false ? nAssignedColor : -1;
				m_iCompTeammateColor.Set( index, nAssignedColor );
			}
		}
		else
			m_iCompTeammateColor.Set( index, -1 );
	}
}