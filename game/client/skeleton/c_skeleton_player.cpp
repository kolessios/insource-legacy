#include "cbase.h"
#include "c_skeleton_player.h"

LINK_ENTITY_TO_CLASS( player, C_SkeletonPlayer );

IMPLEMENT_CLIENTCLASS_DT(C_SkeletonPlayer,DT_SkeletonPlayer,CSkeletonPlayer)
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_SkeletonPlayer )
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()

void C_SkeletonPlayer::OnDataChanged( DataUpdateType_t type )
{
	if ( type == DATA_UPDATE_CREATED )
		SetNextClientThink( CLIENT_THINK_ALWAYS );

	BaseClass::OnDataChanged( type );
}

void C_SkeletonPlayer::UpdateClientSideAnimation()
{
	int curSeq = GetSequence();
	
	Vector vel = GetLocalVelocity();
	//EstimateAbsVelocity( vel );

	int goalSeq = curSeq;

	if ( vel.LengthSqr() > 4 )
	{
		QAngle velAng;
		VectorAngles( vel, velAng );

		goalSeq = SelectWeightedSequence( ACT_RUN );

		float speed = vel.Length2D();
		float yaw = AngleNormalize( -(GetRenderAngles().y - velAng.y) );
		float seqspeed = 150.0f;
		float rate = speed / seqspeed;

		SetPoseParameter( LookupPoseParameter( "move_x" ), cos( DEG2RAD( yaw ) ) * rate );
		SetPoseParameter( LookupPoseParameter( "move_y" ), -sin( DEG2RAD( yaw ) ) * rate );

		SetPlaybackRate( clamp( rate * 0.6f, 1, 1.5f ) );
	}
	else
		goalSeq = SelectWeightedSequence( ACT_IDLE );

	if ( curSeq != goalSeq )
	{
		ResetSequence( goalSeq );
	}

	//m_flAnimTime = gpGlobals->curtime;
	//StudioFrameAdvance();

	if ( GetCycle() >= 1.0f )
		SetCycle( GetCycle() - 1.0f );
}

const QAngle &C_SkeletonPlayer::GetRenderAngles()
{
	m_angRender = GetLocalAngles();
	m_angRender.x = 0;

	return m_angRender;
}