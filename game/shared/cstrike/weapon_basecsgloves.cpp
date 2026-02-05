//========= Copyright PiMoNFeeD, CS:SO, All rights reserved. ==================//
//
// Purpose: Base class for all in-game gloves
//
//=============================================================================//

#include "cbase.h"
#include "weapon_basecsgloves.h"
#include "cs_loadout.h"
#ifdef CLIENT_DLL
#include "c_cs_player.h"
#include "cs_skin_database.h"
#else
#include "cs_player.h"
#endif

LINK_ENTITY_TO_CLASS( cs_base_glove, CBaseCSGloves );
IMPLEMENT_NETWORKCLASS_ALIASED( BaseCSGloves, DT_BaseCSGloves )

BEGIN_NETWORK_TABLE( CBaseCSGloves, DT_BaseCSGloves )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nGloveID ) ),
    RecvPropInt( RECVINFO( iGlovePaintKit )),
#else
	SendPropInt( SENDINFO( m_nGloveID ), 8 ),
    SendPropInt( SENDINFO( iGlovePaintKit), 8 )
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CBaseCSGloves )
END_DATADESC()

CBaseCSGloves::CBaseCSGloves()
{
#ifndef CLIENT_DLL
	m_nGloveID = 0;
    iGlovePaintKit = 0;
#endif
}

#ifdef GAME_DLL
int CBaseCSGloves::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

void CBaseCSGloves::Equip( CBaseAnimating* pOwner )
{
	if ( !pOwner )
		return;

	if ( !pOwner->IsAlive() )
		return;

	FollowEntity( pOwner, true );
	SetOwnerEntity( pOwner );
	AddEffects( EF_BONEMERGE_FASTCULL );
	SetSolid( SOLID_NONE );

	UpdateGlovesModel();

#ifdef CLIENT_DLL
	SetUseParentLightingOrigin( true );
    
    SetClientGlovePaintKit();
    
  /*  CCSPlayer *pPlayer = ToCSPlayer( GetOwnerEntity() );
    int iPaintKit = CSLoadout()->GetGlovesSkinForPlayer(pPlayer, pPlayer->GetTeamNumber());
    
    DevMsg( "[CBaseCSGloves] Paint kit: %d\n", iPaintKit );
    
    if ( iPaintKit <= 0 )
    {
        DevMsg( "[CBaseCSGloves] No paint kit - clearing override\n" );
        ClearMaterialOverride();
        return;
    }
    
    IMaterial *pLeftMaterial = g_SkinDatabase.GetSkinMaterial( iPaintKit );
    IMaterial *pRightMaterial = g_SkinDatabase.GetSkinMaterial( iPaintKit + 1 );

    DevMsg( "[CBaseCSGloves] Left material: %p (%s)\n", 
            pLeftMaterial, 
            pLeftMaterial ? pLeftMaterial->GetName() : "NULL" );
    DevMsg( "[CBaseCSGloves] Right material: %p (%s)\n", 
            pRightMaterial,
            pRightMaterial ? pRightMaterial->GetName() : "NULL" );

    if ( !pLeftMaterial || pLeftMaterial->IsErrorMaterial() )
    {
        Warning( "[CBaseCSGloves] Failed to get left glove material for paint kit %d\n", iPaintKit );
        return;
    }

    if ( !pRightMaterial || pRightMaterial->IsErrorMaterial() )
    {
        Warning( "[CBaseCSGloves] Failed to get right glove material for paint kit %d\n", iPaintKit );
        return;
    }

    DevMsg( "[CBaseCSGloves] Applying materials...\n" );

    SetMaterialOverride( pLeftMaterial, 0 );  
    SetMaterialOverride( pRightMaterial, 1 ); 
    
    DevMsg( "[CBaseCSGloves] Materials applied!\n" );*/
    
    ClearMaterialOverride();
    
    int iPaintKit = GetGlovePaintKit();


    IMaterial *pLeftMaterial = g_SkinDatabase.GetSkinMaterial( iPaintKit );
    IMaterial *pRightMaterial = g_SkinDatabase.GetSkinMaterial( iPaintKit + 1 );
    
    if ( pRightMaterial && !pRightMaterial->IsErrorMaterial() && pLeftMaterial && !pLeftMaterial->IsErrorMaterial() )
    {
        SetMaterialOverride( pLeftMaterial, 0 );  
        SetMaterialOverride( pRightMaterial, 1 ); 
    }
#endif

	// assuming that before equipping them, a DoesModelSupportGloves() check was made
	pOwner->SetBodygroup( pOwner->FindBodygroupByName( "gloves" ), 1 ); // hide default gloves
}
#ifdef CLIENT_DLL
void CBaseCSGloves::SetClientGlovePaintKit()
{
    ClearMaterialOverride();
    
    int iPaintKit = GetGlovePaintKit();


    IMaterial *pLeftMaterial = g_SkinDatabase.GetSkinMaterial( iPaintKit );
    IMaterial *pRightMaterial = g_SkinDatabase.GetSkinMaterial( iPaintKit + 1 );
    
    if ( pRightMaterial && !pRightMaterial->IsErrorMaterial() && pLeftMaterial && !pLeftMaterial->IsErrorMaterial() )
    {
        SetMaterialOverride( pLeftMaterial, 0 );  
        SetMaterialOverride( pRightMaterial, 1 ); 
    }
}
#endif

void CBaseCSGloves::UnEquip()
{
	CCSPlayer *pPlayerOwner = ToCSPlayer( GetOwnerEntity() );

	if ( !pPlayerOwner )
	{
		return;
	}

	pPlayerOwner->SetBodygroup( pPlayerOwner->FindBodygroupByName( "gloves" ), 0 ); // restore default gloves

#ifdef CLIENT_DLL
	ClearMaterialOverride();
#endif

	SetOwnerEntity( NULL );
}

void CBaseCSGloves::UpdateGlovesModel()
{
	CCSPlayer *pPlayerOwner = ToCSPlayer( GetOwnerEntity() );
	if ( !pPlayerOwner )
		return;

#ifdef CLIENT_DLL
	MDLCACHE_CRITICAL_SECTION();
#endif
	const char *pszModel = GetGlovesInfo( m_nGloveID )->szWorldModel;
	SetModel( pszModel );

#ifdef CLIENT_DLL
   SetClientGlovePaintKit(); 
	if ( pPlayerOwner->m_pViewmodelArmConfig != NULL )
		m_nSkin = pPlayerOwner->m_pViewmodelArmConfig->iSkintoneIndex;
	else
#endif
	{
		CStudioHdr *pHdr = pPlayerOwner->GetModelPtr();
		if ( pHdr )
			m_nSkin = GetPlayerViewmodelArmConfigForPlayerModel( pHdr->pszName() )->iSkintoneIndex;
	}
}

#ifdef CLIENT_DLL
void CBaseCSGloves::OnDataChanged( DataUpdateType_t type )
{
    if ( type == DATA_UPDATE_CREATED || type == DATA_UPDATE_DATATABLE_CHANGED )
    {
        ClearMaterialOverride();
        SetClientGlovePaintKit();
    }
}
#endif