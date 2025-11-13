//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side view model implementation. Responsible for drawing
//			the view model.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseviewmodel.h"
#include "model_types.h"
#include "hud.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "view.h"
#include "tier1/convar.h"
#include "mathlib/vmatrix.h"
#include "cl_animevent.h"
#include "eventlist.h"
#include "tools/bonelist.h"
#include <KeyValues.h>
#include "hltvcamera.h"
#ifdef TF_CLIENT_DLL
	#include "tf_weaponbase.h"
#endif
#ifdef CSTRIKE_DLL
	#include "weapon_csbase.h"
	#include "weapon_basecsgrenade.h"
	#include "cs_shareddefs.h"
	#include "c_cs_player.h"
	#include "cs_loadout.h"
#endif

#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif

// NVNT haptics system interface
#include "haptics/ihaptics.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CSTRIKE_DLL
	ConVar cl_righthand( "cl_righthand", "1", FCVAR_ARCHIVE, "Use right-handed view models." );
	ConVar vm_draw_addon( "vm_draw_addon", "1" );
#endif

extern ConVar r_drawviewmodel;

#ifdef TF_CLIENT_DLL
	ConVar cl_flipviewmodels( "cl_flipviewmodels", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_NOT_CONNECTED, "Flip view models." );
#endif

void PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg );

void C_BaseViewModel::UpdateStatTrakGlow( void )
{
	//approach the ideal in 2 seconds
	m_flStatTrakGlowMultiplier = Approach( m_flStatTrakGlowMultiplierIdeal, m_flStatTrakGlowMultiplier, (gpGlobals->frametime * 0.5) );
}

void C_BaseViewModel::OnNewParticleEffect( const char *pszParticleName, CNewParticleEffect *pNewParticleEffect )
{
	if ( FStrEq( pszParticleName, MOLOTOV_PARTICLE_EFFECT_NAME ) )
	{
		m_viewmodelParticleEffect = pNewParticleEffect;
	}
}

void C_BaseViewModel::OnParticleEffectDeleted( CNewParticleEffect *pParticleEffect )
{
	BaseClass::OnParticleEffectDeleted( pParticleEffect );

	if ( m_viewmodelParticleEffect == pParticleEffect )
	{
		m_viewmodelParticleEffect = NULL;
	}
}

void C_BaseViewModel::UpdateParticles()
{
	C_BasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
		return;

	if ( pPlayer->IsPlayerDead() )
		return;

	// Otherwise pass the event to our associated weapon
	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( !pWeapon )
		return;

	CWeaponCSBase *pCSWeapon = ( CWeaponCSBase* )pPlayer->GetActiveWeapon();
	if ( !pCSWeapon )
		return;

	int iWeaponId = pCSWeapon->GetCSWeaponID();

	bool shouldDrawPlayer = ( pPlayer->ShouldDraw() );
	bool visible = r_drawviewmodel.GetBool() && pPlayer && !shouldDrawPlayer;

	if ( visible && iWeaponId == WEAPON_MOLOTOV )
	{
		CBaseCSGrenade *pGren = dynamic_cast<CBaseCSGrenade*>( pPlayer->GetActiveWeapon() );

		if ( pGren->IsPinPulled() )
		{
			//if ( !pGren->IsLoopingSoundPlaying() )
			//{
			//	pGren->SetLoopingSoundPlaying( true );
			//	EmitSound( "Molotov.IdleLoop" );
			//	//DevMsg( 1, "++++++++++>Playing Molotov.IdleLoop 2\n" );
			//}

			// TEST: [mlowrance] This is to test for attachment.
			int iAttachment = -1;
			if ( pWeapon && pWeapon->GetBaseAnimating() )
				iAttachment = pWeapon->GetBaseAnimating()->LookupAttachment( "Wick" );

			if ( iAttachment >= 0 )
			{
				if ( !m_viewmodelParticleEffect )
				{
					DispatchParticleEffect( MOLOTOV_PARTICLE_EFFECT_NAME, PATTACH_POINT_FOLLOW, this, "Wick" );
				}
			}
		}
	}
	else
	{
		if ( m_viewmodelParticleEffect )
		{
			StopSound( "Molotov.IdleLoop" );
			//DevMsg( 1, "---------->Stopping Molotov.IdleLoop 3\n" );
			m_viewmodelParticleEffect->StopEmission( false, true );
			m_viewmodelParticleEffect->SetRemoveFlag();
			m_viewmodelParticleEffect = NULL;
		}
	}
}

void C_BaseViewModel::Simulate()
{
	UpdateParticles();
	UpdateStatTrakGlow();
	BaseClass::Simulate();
	return;
}

void FormatViewModelAttachment( Vector &vOrigin, bool bInverse )
{
	// Presumably, SetUpView has been called so we know our FOV and render origin.
	const CViewSetup *pViewSetup = view->GetPlayerViewSetup();
	
	float worldx = tan( pViewSetup->fov * M_PI/360.0 );
	float viewx = tan( pViewSetup->fovViewmodel * M_PI/360.0 );

	// aspect ratio cancels out, so only need one factor
	// the difference between the screen coordinates of the 2 systems is the ratio
	// of the coefficients of the projection matrices (tan (fov/2) is that coefficient)
	// NOTE: viewx was coming in as 0 when folks set their viewmodel_fov to 0 and show their weapon.
	float factorX = viewx ? ( worldx / viewx ) : 0.0f;
	float factorY = factorX;
	
	// Get the coordinates in the viewer's space.
	Vector tmp = vOrigin - pViewSetup->origin;
	Vector vTransformed( MainViewRight().Dot( tmp ), MainViewUp().Dot( tmp ), MainViewForward().Dot( tmp ) );

	// Now squash X and Y.
	if ( bInverse )
	{
		if ( factorX != 0 && factorY != 0 )
		{
			vTransformed.x /= factorX;
			vTransformed.y /= factorY;
		}
		else
		{
			vTransformed.x = 0.0f;
			vTransformed.y = 0.0f;
		}
	}
	else
	{
		vTransformed.x *= factorX;
		vTransformed.y *= factorY;
	}



	// Transform back to world space.
	Vector vOut = (MainViewRight() * vTransformed.x) + (MainViewUp() * vTransformed.y) + (MainViewForward() * vTransformed.z);
	vOrigin = pViewSetup->origin + vOut;
}


void C_BaseViewModel::FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld )
{
	Vector vecOrigin;
	MatrixPosition( attachmentToWorld, vecOrigin );
	::FormatViewModelAttachment( vecOrigin, false );
	PositionMatrix( vecOrigin, attachmentToWorld );
}


void C_BaseViewModel::UncorrectViewModelAttachment( Vector &vOrigin )
{
	// Unformat the attachment.
	::FormatViewModelAttachment( vOrigin, true );
}


//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
void C_BaseViewModel::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	// We override sound requests so that we can play them locally on the owning player
	if ( ( event == AE_CL_PLAYSOUND ) || ( event == CL_EVENT_SOUND ) )
	{
		// Only do this if we're owned by someone
		if ( GetOwner() != NULL )
		{
			CLocalPlayerFilter filter;
			EmitSound( filter, GetOwner()->GetSoundSourceIndex(), options, &GetAbsOrigin() );
			return;
		}
	}

	// Otherwise pass the event to our associated weapon
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		// NVNT notify the haptics system of our viewmodel's event
		if ( haptics )
			haptics->ProcessHapticEvent(4,"Weapons",pWeapon->GetName(),"AnimationEvents",VarArgs("%i",event));

		bool bResult = pWeapon->OnFireEvent( this, origin, angles, event, options );
		if ( !bResult )
		{
			BaseClass::FireEvent( origin, angles, event, options );
		}
	}
}

bool C_BaseViewModel::Interpolate( float currentTime )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	// Make sure we reset our animation information if we've switch sequences
	UpdateAnimationParity();

	bool bret = BaseClass::Interpolate( currentTime );

	// Hack to extrapolate cycle counter for view model
	float elapsed_time = currentTime - m_flAnimTime;
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// Predicted viewmodels have fixed up interval
	if ( GetPredictable() || IsClientCreated() )
	{
		Assert( pPlayer );
		float curtime = pPlayer ? pPlayer->GetFinalPredictedTime() : gpGlobals->curtime;
		elapsed_time = curtime - m_flAnimTime;
		// Adjust for interpolated partial frame
		if ( !engine->IsPaused() )
		{
			elapsed_time += ( gpGlobals->interpolation_amount * TICK_INTERVAL );
		}
	}

	// Prediction errors?	
	if ( elapsed_time < 0 )
	{
		elapsed_time = 0;
	}

	float dt = elapsed_time * (GetPlaybackRate() * GetSequenceCycleRate( pStudioHdr, GetSequence() )) + m_fCycleOffset;

	if ( dt < 0.0f )
	{
		dt = 0.0f;
	}

	if ( dt >= 1.0f )
	{
		if ( !IsSequenceLooping( GetSequence() ) )
		{
			dt = 0.999f;
		}
		else
		{
			dt = fmod( dt, 1.0f );
		}
	}

	SetCycle( dt );
	return bret;
}


bool C_BaseViewModel::ShouldFlipViewModel()
{
#ifdef CSTRIKE_DLL
	// If cl_righthand is set, then we want them all right-handed.
	CBaseCombatWeapon *pWeapon = m_hWeapon.Get();
	if ( pWeapon )
	{
		const FileWeaponInfo_t *pInfo = &pWeapon->GetWpnData();
		return pInfo->m_bAllowFlipping && pInfo->m_bBuiltRightHanded != cl_righthand.GetBool();
	}
#endif

#ifdef TF_CLIENT_DLL
	CBaseCombatWeapon *pWeapon = m_hWeapon.Get();
	if ( pWeapon )
	{
		return pWeapon->m_bFlipViewModel != cl_flipviewmodels.GetBool();
	}
#endif

	return false;
}


void C_BaseViewModel::ApplyBoneMatrixTransform( matrix3x4_t& transform )
{
	if ( ShouldFlipViewModel() )
	{
		matrix3x4_t viewMatrix, viewMatrixInverse;

		// We could get MATERIAL_VIEW here, but this is called sometimes before the renderer
		// has set that matrix. Luckily, this is called AFTER the CViewSetup has been initialized.
		const CViewSetup *pSetup = view->GetPlayerViewSetup();
		AngleMatrix( pSetup->angles, pSetup->origin, viewMatrixInverse );
		MatrixInvert( viewMatrixInverse, viewMatrix );

		// Transform into view space.
		matrix3x4_t temp, temp2;
		ConcatTransforms( viewMatrix, transform, temp );
		
		// Flip it along X.
		
		// (This is the slower way to do it, and it equates to negating the top row).
		//matrix3x4_t mScale;
		//SetIdentityMatrix( mScale );
		//mScale[0][0] = 1;
		//mScale[1][1] = -1;
		//mScale[2][2] = 1;
		//ConcatTransforms( mScale, temp, temp2 );
		temp[1][0] = -temp[1][0];
		temp[1][1] = -temp[1][1];
		temp[1][2] = -temp[1][2];
		temp[1][3] = -temp[1][3];

		// Transform back out of view space.
		ConcatTransforms( viewMatrixInverse, temp, transform );
	}
}

//-----------------------------------------------------------------------------
// Purpose: check if weapon viewmodel should be drawn
//-----------------------------------------------------------------------------
bool C_BaseViewModel::ShouldDraw()
{
	if ( engine->IsHLTV() )
	{
		return ( HLTVCamera()->GetMode() == OBS_MODE_IN_EYE &&
				 HLTVCamera()->GetPrimaryTarget() == GetOwner()	);
	}
#if defined( REPLAY_ENABLED )
	else if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
	{
		return ( ReplayCamera()->GetMode() == OBS_MODE_IN_EYE &&
				 ReplayCamera()->GetPrimaryTarget() == GetOwner() );
	}
#endif
	else
	{
		return BaseClass::ShouldDraw();
	}
}

#include "cbase.h"
#include "tier1/convar.h"

// ---- Rifles ----
ConVar loadout_ak47_skin("loadout_ak47_skin", "models/weapons/v_models/rif_ak47_skins/v_rif_ak47_redline", FCVAR_ARCHIVE, "AK-47 skin material path");
ConVar loadout_aug_skin("loadout_aug_skin", "", FCVAR_ARCHIVE, "AUG skin material path");
ConVar loadout_awp_skin("loadout_awp_skin", "", FCVAR_ARCHIVE, "AWP skin material path");
ConVar loadout_famas_skin("loadout_famas_skin", "", FCVAR_ARCHIVE, "FAMAS skin material path");
ConVar loadout_galilar_skin("loadout_galilar_skin", "", FCVAR_ARCHIVE, "Galil AR skin material path");
ConVar loadout_m4a1_silenser_skin("loadout_m4a1_silenser_skin", "", FCVAR_ARCHIVE, "M4A1-S skin material path");
ConVar loadout_m4a4_skin("loadout_m4a4_skin", "", FCVAR_ARCHIVE, "M4A4 skin material path");
ConVar loadout_sg556_skin("loadout_sg556_skin", "", FCVAR_ARCHIVE, "SG556 skin material path");

// ---- SMG ----
ConVar loadout_bizon_skin("loadout_bizon_skin", "", FCVAR_ARCHIVE, "PP-Bizon skin material path");
ConVar loadout_mac10_skin("loadout_mac10_skin", "", FCVAR_ARCHIVE, "MAC-10 skin material path");
ConVar loadout_mp5sd_skin("loadout_mp5sd_skin", "", FCVAR_ARCHIVE, "MP5-SD skin material path");
ConVar loadout_mp9_skin("loadout_mp9_skin", "", FCVAR_ARCHIVE, "MP9 skin material path");
ConVar loadout_p90_skin("loadout_p90_skin", "", FCVAR_ARCHIVE, "P90 skin material path");
ConVar loadout_ump45_skin("loadout_ump45_skin", "", FCVAR_ARCHIVE, "UMP-45 skin material path");

// ---- Heavy ----
ConVar loadout_m249_skin("loadout_m249_skin", "", FCVAR_ARCHIVE, "M249 skin material path");
ConVar loadout_negev_skin("loadout_negev_skin", "", FCVAR_ARCHIVE, "Negev skin material path");
ConVar loadout_mag7_skin("loadout_mag7_skin", "", FCVAR_ARCHIVE, "MAG-7 skin material path");
ConVar loadout_nova_skin("loadout_nova_skin", "", FCVAR_ARCHIVE, "Nova skin material path");
ConVar loadout_sawedoff_skin("loadout_sawedoff_skin", "", FCVAR_ARCHIVE, "Sawed-Off skin material path");
ConVar loadout_xm1014_skin("loadout_xm1014_skin", "", FCVAR_ARCHIVE, "XM1014 skin material path");

// ---- Pistols ----
ConVar loadout_cz75a_skin("loadout_cz75a_skin", "", FCVAR_ARCHIVE, "CZ75-Auto skin material path");
ConVar loadout_deagle_skin("loadout_deagle_skin", "", FCVAR_ARCHIVE, "Desert Eagle skin material path");
ConVar loadout_elite_skin("loadout_elite_skin", "", FCVAR_ARCHIVE, "Dual Berettas skin material path");
ConVar loadout_fiveseven_skin("loadout_fiveseven_skin", "", FCVAR_ARCHIVE, "Five-SeveN skin material path");
ConVar loadout_glock_skin("loadout_glock_skin", "", FCVAR_ARCHIVE, "Glock-18 skin material path");
ConVar loadout_hkp2000_skin("loadout_hkp2000_skin", "", FCVAR_ARCHIVE, "P2000 skin material path");
ConVar loadout_p250_skin("loadout_p250_skin", "", FCVAR_ARCHIVE, "P250 skin material path");
ConVar loadout_revolver_skin("loadout_revolver_skin", "", FCVAR_ARCHIVE, "R8 Revolver skin material path");
ConVar loadout_tec9_skin("loadout_tec9_skin", "", FCVAR_ARCHIVE, "Tec-9 skin material path");
ConVar loadout_usp_silencer_skin("loadout_usp_silencer_skin", "", FCVAR_ARCHIVE, "USP-S skin material path");

// ---- Snipers ----
ConVar loadout_g3sg1_skin("loadout_g3sg1_skin", "", FCVAR_ARCHIVE, "G3SG1 skin material path");
ConVar loadout_scar20_skin("loadout_scar20_skin", "", FCVAR_ARCHIVE, "SCAR-20 skin material path");
ConVar loadout_ssg08_skin("loadout_ssg08_skin", "", FCVAR_ARCHIVE, "SSG 08 skin material path");

// ---- Knives ----
ConVar loadout_knife_bayonet_skin("loadout_knife_bayonet_skin", "", FCVAR_ARCHIVE, "Bayonet skin material path");
ConVar loadout_knife_butterfly_skin("loadout_knife_butterfly_skin", "", FCVAR_ARCHIVE, "Butterfly Knife skin material path");
ConVar loadout_knife_cains_skin("loadout_knife_cains_skin", "", FCVAR_ARCHIVE, "Cains Knife skin material path");
ConVar loadout_knife_cord_skin("loadout_knife_cord_skin", "", FCVAR_ARCHIVE, "Cord Knife skin material path");
ConVar loadout_knife_css_skin("loadout_knife_css_skin", "", FCVAR_ARCHIVE, "Classic Knife skin material path");
ConVar loadout_knife_falshion_skin("loadout_knife_falshion_skin", "", FCVAR_ARCHIVE, "Falchion Knife skin material path");
ConVar loadout_knife_flip_skin("loadout_knife_flip_skin", "", FCVAR_ARCHIVE, "Flip Knife skin material path");
ConVar loadout_knife_gut_skin("loadout_knife_gut_skin", "", FCVAR_ARCHIVE, "Gut Knife skin material path");
ConVar loadout_knife_gypsy_jackknife_skin("loadout_knife_gypsy_jackknife_skin", "", FCVAR_ARCHIVE, "Navaja Knife skin material path");
ConVar loadout_knife_karambit_skin("loadout_knife_karambit_skin", "", FCVAR_ARCHIVE, "Karambit skin material path");
ConVar loadout_knife_m9_bayonet_skin("loadout_knife_m9_bayonet_skin", "", FCVAR_ARCHIVE, "M9 Bayonet skin material path");
ConVar loadout_knife_outdoor_skin("loadout_knife_outdoor_skin", "", FCVAR_ARCHIVE, "Outdoor Knife skin material path");
ConVar loadout_knife_push_skin("loadout_knife_push_skin", "", FCVAR_ARCHIVE, "Shadow Daggers skin material path");
ConVar loadout_knife_skeleton_skin("loadout_knife_skeleton_skin", "", FCVAR_ARCHIVE, "Skeleton Knife skin material path");
ConVar loadout_knife_stiletto_skin("loadout_knife_stiletto_skin", "", FCVAR_ARCHIVE, "Stiletto Knife skin material path");
ConVar loadout_knife_survival_bowie_skin("loadout_knife_survival_bowie_skin", "", FCVAR_ARCHIVE, "Bowie Knife skin material path");
ConVar loadout_knife_tactical_skin("loadout_knife_tactical_skin", "", FCVAR_ARCHIVE, "Huntsman Knife skin material path");
ConVar loadout_knife_ursus_skin("loadout_knife_ursus_skin", "", FCVAR_ARCHIVE, "Ursus Knife skin material path");
ConVar loadout_knife_widowmaker_skin("loadout_knife_widowmaker_skin", "", FCVAR_ARCHIVE, "Widowmaker Knife skin material path");

struct WeaponSkin_t
{
    const char* pszWeaponClass;
    ConVar* pSkinConVar;
    IMaterial* pMaterial;
    CUtlString sLastSkin;

    WeaponSkin_t(const char* weapon, ConVar* cvar)
        : pszWeaponClass(weapon), pSkinConVar(cvar), pMaterial(nullptr) {}
};

//-----------------------------------------------------------------------------
// Purpose: Render the weapon. Draw the Viewmodel if the weapon's being carried
//			by this player, otherwise draw the worldmodel.
//-----------------------------------------------------------------------------
int C_BaseViewModel::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	CMatRenderContextPtr pRenderContext( materials );

	if ( flags & STUDIO_RENDER )
	{
		// Determine blending amount and tell engine
		float blend = (float)( GetFxBlend() / 255.0f );

		// Totally gone
		if ( blend <= 0.0f )
			return 0;

		// Tell engine
		render->SetBlend( blend );

		float color[3];
		GetColorModulation( color );
		render->SetColorModulation(	color );
	}

	if ( ShouldFlipViewModel() )
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );
		
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	int ret;
	// If the local player's overriding the viewmodel rendering, let him do it
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		ret = pPlayer->DrawOverriddenViewmodel( this, flags );
	}
	else if ( pWeapon && pWeapon->IsOverridingViewmodel() )
	{
		ret = pWeapon->DrawOverriddenViewmodel( this, flags );
	}
	else
	{
		ret = BaseClass::DrawModel( flags );
	}

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	// Now that we've rendered, reset the animation restart flag
	if ( flags & STUDIO_RENDER )
	{
		if ( m_nOldAnimationParity != m_nAnimationParity )
		{
			m_nOldAnimationParity = m_nAnimationParity;
		}
		// Tell the weapon itself that we've rendered, in case it wants to do something
		if ( pWeapon )
		{
			pWeapon->ViewModelDrawn( this );
		}
	}


	if ( flags && vm_draw_addon.GetBool() )
	{
		FOR_EACH_VEC( m_vecViewmodelArmModels, i )
		{
			if ( m_vecViewmodelArmModels[i] )
			{

				if ( m_vecViewmodelArmModels[i]->GetMoveParent() != this )
				{
					m_vecViewmodelArmModels[i]->SetEFlags( EF_BONEMERGE );
					m_vecViewmodelArmModels[i]->SetParent( this );
				}

				m_vecViewmodelArmModels[i]->DrawModel( flags );
			}
		}
		if ( m_viewmodelStatTrakAddon )
		{
			m_viewmodelStatTrakAddon->DrawModel( flags );
		}
	}
	
	if (flags & STUDIO_RENDER)
{
    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();
    if (!pLocal)
        return ret;

    C_WeaponCSBase* pWeapon = dynamic_cast<C_WeaponCSBase*>(pLocal->GetActiveWeapon());
    if (!pWeapon)
        return ret;

    if (pWeapon->GetOriginalOwnerIndex() != pLocal->entindex())
    {
        return BaseClass::DrawModel(flags);
    }

    const char* pszClass = pWeapon->GetClassname();
    if (!pszClass)
        return ret;

    IMaterial* pSkinMat = nullptr;

    for (auto& ws : g_WeaponSkins)
    {
        if (Q_stricmp(pszClass, ws.pszWeaponClass) == 0)
        {
            const char* skinPath = ws.pSkinConVar->GetString();

            if (!skinPath || !skinPath[0])
                break;

            if (!ws.pMaterial || ws.sLastSkin != skinPath)
            {
                ws.sLastSkin = skinPath;
                ws.pMaterial = materials->FindMaterial(skinPath, TEXTURE_GROUP_MODEL, true);

                if (!ws.pMaterial || IsErrorMaterial(ws.pMaterial))
                {
                    Warning("Failed to load skin: %s for weapon: %s\n", skinPath, pszClass);
                    ws.pMaterial = nullptr;
                    break;
                }

                ws.pMaterial->IncrementReferenceCount();

                MaterialLock_t hLock = materials->Lock();
                ws.pMaterial->RefreshPreservingMaterialVars();
                materials->Unlock(hLock);
            }

            if (ws.pMaterial && !ws.pMaterial->IsPrecached())
            {
                MaterialLock_t hLock = materials->Lock();
                ws.pMaterial->Refresh();
                materials->Unlock(hLock);
            }

            pSkinMat = ws.pMaterial;
            break;
        }
    }

    if (!pSkinMat)
        return BaseClass::DrawModel(flags);

    modelrender->ForcedMaterialOverride(pSkinMat);
    int skinRet = BaseClass::DrawModel(flags);
    modelrender->ForcedMaterialOverride(nullptr);

    return skinRet;
}

	return ret;
}

// ---- Gloves skin ConVars ----
ConVar loadout_glove_bloodhound_skin("loadout_glove_bloodhound_skin", "", FCVAR_ARCHIVE, "Material path for Bloodhound gloves (viewmodel)");
ConVar loadout_glove_fingerless_skin("loadout_glove_fingerless_skin", "", FCVAR_ARCHIVE, "Material path for Fingerless gloves");
ConVar loadout_glove_fullfinger_skin("loadout_glove_fullfinger_skin", "", FCVAR_ARCHIVE, "Material path for Fullfinger gloves");
ConVar loadout_glove_handwrap_leathery_skin("loadout_glove_handwrap_leathery_skin", "", FCVAR_ARCHIVE, "Material path for Handwrap (leathery)");
ConVar loadout_glove_hardknuckle_skin("loadout_glove_hardknuckle_skin", "", FCVAR_ARCHIVE, "Material path for Hardknuckle gloves");
ConVar loadout_glove_hardknuckle_black_skin("loadout_glove_hardknuckle_black_skin", "", FCVAR_ARCHIVE, "Material path for Hardknuckle Black gloves");
ConVar loadout_glove_hardknuckle_blue_skin("loadout_glove_hardknuckle_blue_skin", "", FCVAR_ARCHIVE, "Material path for Hardknuckle Blue gloves");
ConVar loadout_glove_motorcycle_skin("loadout_glove_motorcycle_skin", "", FCVAR_ARCHIVE, "Material path for Motorcycle gloves");
ConVar loadout_glove_slick_skin("loadout_glove_slick_skin", "", FCVAR_ARCHIVE, "Material path for Slick gloves");
ConVar loadout_glove_specialist_skin("loadout_glove_specialist_skin", "", FCVAR_ARCHIVE, "Material path for Specialist gloves");
ConVar loadout_glove_sporty_skin("loadout_glove_sporty_skin", "", FCVAR_ARCHIVE, "Material path for Sporty gloves");
ConVar loadout_glove_sas_old_skin("loadout_glove_sas_old_skin", "", FCVAR_ARCHIVE, "Material path for SAS old gloves");
ConVar loadout_glove_fbi_old_skin("loadout_glove_fbi_old_skin", "", FCVAR_ARCHIVE, "Material path for FBI old gloves");
ConVar loadout_glove_phoenix_old_skin("loadout_glove_phoenix_old_skin", "", FCVAR_ARCHIVE, "Material path for Phoenix old gloves");
ConVar loadout_glove_leet_old_skin("loadout_glove_leet_old_skin", "", FCVAR_ARCHIVE, "Material path for Leet old gloves");
ConVar loadout_glove_bare_hands_skin("loadout_glove_bare_hands_skin", "", FCVAR_ARCHIVE, "Material path for Bare hands (v_bare_hands)");

struct GloveSkin_t
{
    const char* pszVModelName;   
    ConVar*     pConVar;         
    IMaterial*  pMaterial;       
    CUtlString  sLastSkin;       

    GloveSkin_t(const char* vm, ConVar* cv) : pszVModelName(vm), pConVar(cv), pMaterial(nullptr), sLastSkin() {}
};

static GloveSkin_t g_GloveSkins[] = {
    { "models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound.mdl", &loadout_glove_bloodhound_skin },
    { "models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound_perfectworld.mdl", &loadout_glove_bloodhound_skin }, // alias
    { "models/weapons/v_models/arms/glove_fingerless/v_glove_fingerless.mdl", &loadout_glove_fingerless_skin },
    { "models/weapons/v_models/arms/glove_fullfinger/v_glove_fullfinger.mdl", &loadout_glove_fullfinger_skin },
    { "models/weapons/v_models/arms/glove_handwrap_leathery/v_glove_handwrap_leathery.mdl", &loadout_glove_handwrap_leathery_skin },
    { "models/weapons/v_models/arms/glove_hardknuckle/v_glove_hardknuckle.mdl", &loadout_glove_hardknuckle_skin },
    { "models/weapons/v_models/arms/glove_hardknuckle/v_glove_hardknuckle_black.mdl", &loadout_glove_hardknuckle_black_skin },
    { "models/weapons/v_models/arms/glove_hardknuckle/v_glove_hardknuckle_blue.mdl", &loadout_glove_hardknuckle_blue_skin },
    { "models/weapons/v_models/arms/glove_motorcycle/v_glove_motorcycle.mdl", &loadout_glove_motorcycle_skin },
    { "models/weapons/v_models/arms/glove_slick/v_glove_slick.mdl", &loadout_glove_slick_skin },
    { "models/weapons/v_models/arms/glove_specialist/v_glove_specialist.mdl", &loadout_glove_specialist_skin },
    { "models/weapons/v_models/arms/glove_sporty/v_glove_sporty.mdl", &loadout_glove_sporty_skin },
    { "models/weapons/v_models/arms/glove_sas_old/v_glove_sas_old.mdl", &loadout_glove_sas_old_skin },
    { "models/weapons/v_models/arms/glove_fbi_old/v_glove_fbi_old.mdl", &loadout_glove_fbi_old_skin },
    { "models/weapons/v_models/arms/glove_phoenix_old/v_glove_phoenix_old.mdl", &loadout_glove_phoenix_old_skin },
    { "models/weapons/v_models/arms/glove_leet_old/v_glove_leet_old.mdl", &loadout_glove_leet_old_skin },
    { "models/weapons/v_models/arms/bare/v_bare_hands.mdl", &loadout_glove_bare_hands_skin }
};
static const int g_nGloveSkinsCount = ARRAYSIZE(g_GloveSkins);

static GloveSkin_t* FindGloveSkinEntryForModelName(const char* pszModelName)
{
    if (!pszModelName || !pszModelName[0]) return nullptr;

    for (int i = 0; i < g_nGloveSkinsCount; ++i)
    {
        if (Q_stricmp(pszModelName, g_GloveSkins[i].pszVModelName) == 0)
            return &g_GloveSkins[i];
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BaseViewModel::InternalDrawModel( int flags )
{
    CMatRenderContextPtr pRenderContext( materials );
    if ( ShouldFlipViewModel() )
        pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

    // --- BEGIN: glove material override logic ---
    IMaterial* pOverrideMat = nullptr;

    const model_t* pModel = modelinfo->GetModel( GetModelIndex() );
    const char* pszModelName = pModel ? modelinfo->GetModelName( pModel ) : nullptr;

    if ( pszModelName )
    {
        GloveSkin_t* pEntry = FindGloveSkinEntryForModelName( pszModelName );
        if ( pEntry && pEntry->pConVar )
        {
            const char* pszMatPath = pEntry->pConVar->GetString();
            if ( pszMatPath && pszMatPath[0] != '\0' )
            {
                if ( !pEntry->pMaterial || V_stricmp( pEntry->sLastSkin.String(), pszMatPath ) != 0 )
                {
                    pEntry->sLastSkin = pszMatPath;
                    pEntry->pMaterial = materials->FindMaterial( pszMatPath, TEXTURE_GROUP_MODEL, true );
                    if ( pEntry->pMaterial && !pEntry->pMaterial->IsErrorMaterial() )
                    {
                        pEntry->pMaterial->IncrementReferenceCount();
                    }
                    else
                    {
                        pEntry->pMaterial = nullptr;
                    }
                }

                if ( pEntry->pMaterial && !pEntry->pMaterial->IsErrorMaterial() )
                    pOverrideMat = pEntry->pMaterial;
            }
        }
    }

    if ( pOverrideMat )
        modelrender->ForcedMaterialOverride( pOverrideMat );

    int ret = BaseClass::InternalDrawModel( flags );

    if ( pOverrideMat )
        modelrender->ForcedMaterialOverride( nullptr );
    // --- END override logic ---

    pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

    return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Called by the player when the player's overriding the viewmodel drawing. Avoids infinite recursion.
//-----------------------------------------------------------------------------
int C_BaseViewModel::DrawOverriddenViewmodel( int flags )
{
	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseViewModel::GetFxBlend( void )
{
	// See if the local player wants to override the viewmodel's rendering
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		pPlayer->ComputeFxBlend();
		return pPlayer->GetFxBlend();
	}

	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->IsOverridingViewmodel() )
	{
		pWeapon->ComputeFxBlend();
		return pWeapon->GetFxBlend();
	}

	return BaseClass::GetFxBlend();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseViewModel::IsTransparent( void )
{
	// See if the local player wants to override the viewmodel's rendering
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		return pPlayer->ViewModel_IsTransparent();
	}

	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->IsOverridingViewmodel() )
		return pWeapon->ViewModel_IsTransparent();

	return BaseClass::IsTransparent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseViewModel::UsesPowerOfTwoFrameBufferTexture( void )
{
	// See if the local player wants to override the viewmodel's rendering
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		return pPlayer->ViewModel_IsUsingFBTexture();
	}

	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->IsOverridingViewmodel() )
	{
		return pWeapon->ViewModel_IsUsingFBTexture();
	}

	return BaseClass::UsesPowerOfTwoFrameBufferTexture();
}

//-----------------------------------------------------------------------------
// Purpose: If the animation parity of the weapon has changed, we reset cycle to avoid popping
//-----------------------------------------------------------------------------
void C_BaseViewModel::UpdateAnimationParity( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	
	// If we're predicting, then we don't use animation parity because we change the animations on the clientside
	// while predicting. When not predicting, only the server changes the animations, so a parity mismatch
	// tells us if we need to reset the animation.
	if ( m_nOldAnimationParity != m_nAnimationParity && !GetPredictable() )
	{
		float curtime = (pPlayer && IsIntermediateDataAllocated()) ? pPlayer->GetFinalPredictedTime() : gpGlobals->curtime;
		// FIXME: this is bad
		// Simulate a networked m_flAnimTime and m_flCycle
		// FIXME:  Do we need the magic 0.1?
		SetCycle( 0.0f ); // GetSequenceCycleRate( GetSequence() ) * 0.1;
		m_flAnimTime = curtime;
		m_fCycleOffset = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update global map state based on data received
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BaseViewModel::OnDataChanged( DataUpdateType_t updateType )
{
	SetPredictionEligible( true );
	BaseClass::OnDataChanged(updateType);
}

void C_BaseViewModel::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate(updateType);
	OnLatchInterpolatedVariables( LATCH_ANIMATION_VAR );
}


//-----------------------------------------------------------------------------
// Purpose: Add entity to visible view models list
//-----------------------------------------------------------------------------
void C_BaseViewModel::AddEntity( void )
{
	// Server says don't interpolate this frame, so set previous info to new info.
	if ( IsNoInterpolationFrame() )
	{
		ResetLatched();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseViewModel::GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS])
{
	BaseClass::GetBoneControllers( controllers );

	// Tell the weapon itself that we've rendered, in case it wants to do something
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->GetViewmodelBoneControllers( this, controllers );
	}
}

void C_BaseViewModel::UpdateAllViewmodelAddons( void )
{
	C_CSPlayer *pPlayer = dynamic_cast<C_CSPlayer*>( GetOwner() );

	// Remove any view model add ons if we're spectating.
	if ( !pPlayer )
	{
		RemoveViewmodelArmModels();
		RemoveViewmodelStatTrak();
		return;
	}

	CWeaponCSBase* pCSWeapon = dynamic_cast<CWeaponCSBase*>( pPlayer->GetActiveWeapon() );
	if ( !pCSWeapon )
	{
		RemoveViewmodelArmModels();
		RemoveViewmodelStatTrak();
		return;
	}

	int weaponID = pCSWeapon->GetCSWeaponID();

	// Note: only arms race (gun game) knives change their bodygroup to indicate their team.
	if ( weaponID == WEAPON_KNIFE_GG )
	{
		int bodyPartID = (pPlayer->GetTeamNumber() == TEAM_TERRORIST) ? 0 : 1;
		SetBodygroup( 0, bodyPartID );
	}

	if ( pPlayer->m_pViewmodelArmConfig == NULL )
	{
		RemoveViewmodelArmModels();

		CStudioHdr *pHdr = pPlayer->GetModelPtr();
		if ( pHdr )
		{
			pPlayer->m_pViewmodelArmConfig = GetPlayerViewmodelArmConfigForPlayerModel( pHdr->pszName() );
		}
	}

	if ( pPlayer->m_bNeedToChangeGloves )
		RemoveViewmodelArmModels();
	
	// add gloves and sleeves
	if ( pPlayer->m_pViewmodelArmConfig != NULL && m_vecViewmodelArmModels.Count() == 0 )
	{
		if ( CSLoadout()->HasGlovesSet( pPlayer, pPlayer->GetTeamNumber() ) )
		{
			AddViewmodelArmModel( GetGlovesInfo( CSLoadout()->GetGlovesForPlayer( pPlayer, pPlayer->GetTeamNumber() ) )->szViewModel, pPlayer->m_pViewmodelArmConfig->iSkintoneIndex, pPlayer->m_pViewmodelArmConfig->bHideBareArms );
			if ( pPlayer->m_pViewmodelArmConfig->szAssociatedSleeveModelGloveOverride[0] != NULL )
				AddViewmodelArmModel( pPlayer->m_pViewmodelArmConfig->szAssociatedSleeveModelGloveOverride );
			else
				AddViewmodelArmModel( pPlayer->m_pViewmodelArmConfig->szAssociatedSleeveModel );
		}
		else
		{
			AddViewmodelArmModel( pPlayer->m_pViewmodelArmConfig->szAssociatedGloveModel, pPlayer->m_pViewmodelArmConfig->iSkintoneIndex, pPlayer->m_pViewmodelArmConfig->bHideBareArms );
			AddViewmodelArmModel( pPlayer->m_pViewmodelArmConfig->szAssociatedSleeveModel );
		}
	}

	// verify stattrak module and add if necessary
	if ( pCSWeapon->HasStatTrak() )
	{
		int iEntIndex = pPlayer->entindex();
		if ( pPlayer->IsControllingBot() )
			iEntIndex = pPlayer->GetControlledBotIndex();

		AddViewmodelStatTrak( pCSWeapon, iEntIndex );
	}
	else
		RemoveViewmodelStatTrak();
}

//--------------------------------------------------------------------------------------------------------
void C_BaseViewModel::AddViewmodelArmModel( const char *pszArmsModel, int nSkintoneIndex, bool bHideBareArms )
{
	// Only create the view model attachment if we have a valid arm model
	if ( pszArmsModel == NULL || pszArmsModel[0] == '\0' || modelinfo->GetModelIndex( pszArmsModel ) == -1 )
		return;

	C_ViewmodelAttachmentModel *pEnt = new class C_ViewmodelAttachmentModel;
	if ( pEnt && pEnt->InitializeAsClientEntity( pszArmsModel, RENDER_GROUP_VIEW_MODEL_OPAQUE ) )
	{
		m_vecViewmodelArmModels[ m_vecViewmodelArmModels.AddToTail() ] = pEnt;

		if ( nSkintoneIndex != -1 )
			pEnt->m_nSkin = nSkintoneIndex;

		// PiMoN: magic trick to get 0.01 more fps on potato PCs
		int iBodygroup = pEnt->FindBodygroupByName( "bare" );
		if ( iBodygroup != -1 )
			pEnt->SetBodygroup( iBodygroup, bHideBareArms );

		pEnt->SetParent( this );
		pEnt->SetLocalOrigin( vec3_origin );
		pEnt->UpdatePartitionListEntry();
		pEnt->CollisionProp()->MarkPartitionHandleDirty();
		pEnt->UpdateVisibility();
		pEnt->SetViewmodel( this );
		pEnt->SetUseParentLightingOrigin( true );

		RemoveEffects( EF_NODRAW );
	}
}

void C_BaseViewModel::AddViewmodelStatTrak( CWeaponCSBase *pWeapon, int holderIndex )
{
	if ( m_viewmodelStatTrakAddon && m_viewmodelStatTrakAddon.Get() && m_viewmodelStatTrakAddon->GetMoveParent() )
 		return;

	RemoveViewmodelStatTrak();

	if (!pWeapon)
		return;

	if ( !pWeapon->GetCSWpnData().m_szStatTrakModel && !pWeapon->GetCSWpnData().m_szStatTrakModel[0] )
		return;

	C_ViewmodelAttachmentModel *pStatTrakEnt = new class C_ViewmodelAttachmentModel;
	if ( pStatTrakEnt && pStatTrakEnt->InitializeAsClientEntity( pWeapon->GetCSWpnData().m_szStatTrakModel, RENDER_GROUP_VIEW_MODEL_OPAQUE ) )
	{
		m_viewmodelStatTrakAddon = pStatTrakEnt;
		pStatTrakEnt->SetParent( this );
		pStatTrakEnt->SetLocalOrigin( vec3_origin );
		pStatTrakEnt->UpdatePartitionListEntry();
		pStatTrakEnt->CollisionProp()->MarkPartitionHandleDirty();
		pStatTrakEnt->UpdateVisibility();
		pStatTrakEnt->SetViewmodel( this );
		pStatTrakEnt->SetUseParentLightingOrigin( true );

		if ( !cl_righthand.GetBool() )
		{
			pStatTrakEnt->SetBodygroup( 0, 1 ); // use a special mirror-image stattrak module that appears correct for lefties
		}

		// this stat trak weapon doesn't belong to the current holder, display error message on the digital display. This is impossible for knives
		if ( !CSLoadout()->IsKnife( pWeapon->GetCSWeaponID() ) )
		{
			if ( pWeapon->GetOriginalOwnerIndex() != holderIndex )
			{
				pStatTrakEnt->SetBodygroup( 1, cl_righthand.GetBool() ? 1 : 2 ); // show the error screen bodygroup
			}
		}
	}
}

void C_BaseViewModel::RemoveViewmodelArmModels( void )
{
	FOR_EACH_VEC_BACK( m_vecViewmodelArmModels, i )
	{
		C_ViewmodelAttachmentModel *pEnt = m_vecViewmodelArmModels[i].Get();
		if ( pEnt )
		{
			pEnt->Remove();
		}
	}
	m_vecViewmodelArmModels.RemoveAll();
}

void C_BaseViewModel::RemoveViewmodelStatTrak( void )
{
	C_ViewmodelAttachmentModel *pStatTrakEnt = m_viewmodelStatTrakAddon.Get();
	if ( pStatTrakEnt )
	{
		pStatTrakEnt->Remove();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : RenderGroup_t
//-----------------------------------------------------------------------------
RenderGroup_t C_BaseViewModel::GetRenderGroup()
{
	return RENDER_GROUP_VIEW_MODEL_OPAQUE;
}


bool C_ViewmodelAttachmentModel::InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup )
{
	if ( !BaseClass::InitializeAsClientEntity( pszModelName, renderGroup ) )
		return false;

	AddEffects( EF_BONEMERGE );
	AddEffects( EF_BONEMERGE_FASTCULL );

	// Invisible by default, and made visible->drawn->made invisible when the viewmodel is drawn
	AddEffects( EF_NODRAW );
	return true;
}

void C_ViewmodelAttachmentModel::SetViewmodel( C_BaseViewModel *pVM )
{
	m_hViewmodel = pVM;
}

int C_ViewmodelAttachmentModel::InternalDrawModel( int flags )
{
	CMatRenderContextPtr pRenderContext( materials );
	C_BaseViewModel *pViewmodel = m_hViewmodel;
	if ( pViewmodel && pViewmodel->ShouldFlipModel() )
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

	int r = BaseClass::InternalDrawModel( flags );

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	return r;
}