#include "cbase.h"
#include "skeleton_player.h"

LINK_ENTITY_TO_CLASS( player, CSkeletonPlayer );

IMPLEMENT_SERVERCLASS_ST(CSkeletonPlayer,DT_SkeletonPlayer)
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
END_SEND_TABLE()

void CSkeletonPlayer::Spawn()
{
	// Dying without a player model crashes the client
	SetModel("models/swarm/marine/marine.mdl");

	BaseClass::Spawn();

	UseClientSideAnimation();
}

void CSkeletonPlayer::CreateViewModel( int index )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel(index) )
		return;

	CPredictedViewModel* vm = (CPredictedViewModel*)CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

void CSkeletonPlayer::PostThink()
{
	BaseClass::PostThink();

	// Keep the model upright; pose params will handle pitch aiming.
	//QAngle angles = GetLocalAngles();
	//angles[PITCH] = 0;
	//SetLocalAngles(angles);
}

#include "..\server\ilagcompensationmanager.h"

void CSkeletonPlayer::FireBullets ( const FireBulletsInfo_t &info )
{
	lagcompensation->StartLagCompensation( this, LAG_COMPENSATE_HITBOXES);

	BaseClass::FireBullets(info);

	lagcompensation->FinishLagCompensation( this );
}