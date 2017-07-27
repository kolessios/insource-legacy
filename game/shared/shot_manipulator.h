//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SHOT_MANIPULATOR_H
#define SHOT_MANIPULATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

extern ConVar ai_shot_bias_min;
extern ConVar ai_shot_bias_max;

//---------------------------------------------------------
// Caches off a shot direction and allows you to perform
// various operations on it without having to recalculate
// vecRight and vecUp each time. 
//---------------------------------------------------------
class CShotManipulator
{
public:
	CShotManipulator( const Vector &vecForward )
	{
		SetShootDir( vecForward );
	};

	void SetShootDir( const Vector &vecForward )
	{
		m_vecShotDirection = vecForward;
		VectorVectors( m_vecShotDirection, m_vecRight, m_vecUp );
	}

	const Vector &ApplySpreadLegacy( const Vector &vecSpread, float bias = 1.0 );
	const Vector &ApplySpreadBias( const Vector &vecSpread, float bias = 1.0 );
	const Vector &ApplySpread( const Vector &vecSpread, float bias = 1.0 );

	const Vector &GetShotDirection()	{ return m_vecShotDirection; }
	const Vector &GetResult()			{ return m_vecResult; }
	const Vector &GetRightVector()		{ return m_vecRight; }
	const Vector &GetUpVector()			{ return m_vecUp;}

private:
	Vector m_vecShotDirection;
	Vector m_vecRight;
	Vector m_vecUp;
	Vector m_vecResult;
};
// -------------------------------------------------------------------------------------
// CVB - holtt - Replace the normal ApplySpread with a call to either Valve's original
//               code, or a new bias code set that puts more shots in the center of
//               the shot spread.
// -------------------------------------------------------------------------------------
inline const Vector &CShotManipulator::ApplySpread ( const Vector &vecSpread, float bias )
{
	// return ApplySpreadVALVE (vecSpread, bias);
	return ApplySpreadBias(vecSpread, bias);
}

//---------------------------------------------------------
// CVB - holtt - Renamed this as we are going to replace it
//               with a new version that biases shots
//               towards the middle instead of random.
//---------------------------------------------------------
// Take a vector (direction) and another vector (spread) 
// and modify the direction to point somewhere within the 
// spread. This used to live inside FireBullets.
//---------------------------------------------------------
inline const Vector &CShotManipulator::ApplySpreadLegacy( const Vector &vecSpread, float bias )
{
	// get circular gaussian spread
	float x, y, z;

	if ( bias > 1.0 )
		bias = 1.0;
	else if ( bias < 0.0 )
		bias = 0.0;

	float shotBiasMin = ai_shot_bias_min.GetFloat();
	float shotBiasMax = ai_shot_bias_max.GetFloat();

	// 1.0 gaussian, 0.0 is flat, -1.0 is inverse gaussian
	float shotBias = ( ( shotBiasMax - shotBiasMin ) * bias ) + shotBiasMin;

	float flatness = ( fabsf(shotBias) * 0.5 );

	do
	{
		x = random->RandomFloat(-1,1) * flatness + random->RandomFloat(-1,1) * (1 - flatness);
		y = random->RandomFloat(-1,1) * flatness + random->RandomFloat(-1,1) * (1 - flatness);
		if ( shotBias < 0 )
		{
			x = ( x >= 0 ) ? 1.0 - x : -1.0 - x;
			y = ( y >= 0 ) ? 1.0 - y : -1.0 - y;
		}
		z = x*x+y*y;
	} while (z > 1);

	m_vecResult = m_vecShotDirection + x * vecSpread.x * m_vecRight + y * vecSpread.y * m_vecUp;

	return m_vecResult;
}

//---------------------------------------------------------
// CVB - holtt - This version creates shot patterns that
//               are biased towards the middle of the cone.
//---------------------------------------------------------
// Take a vector (direction) and another vector (spread) 
// and modify the direction to point somewhere within the 
// spread. This used to live inside FireBullets.
//---------------------------------------------------------
inline const Vector &CShotManipulator::ApplySpreadBias( const Vector &vecSpread, float bias )
{
	// get circular gaussian spread
	float x, y;
	float r, theta;
	float sinTheta, cosTheta;

	// Generate a random polar coordinate
	r = random->RandomFloat (0,1);
	theta = random->RandomFloat (0, DEG2RAD(360));

	// ---------------------------------------------------------------------------------
	// Note - this polar coordinate we created does not create a random point spread
	// inside the unit circle.  The key is the radius (R) randomization.  Half of
	// our random radius points will lie between 0.0 and 0.5, and the other half
	// from 0.5 to 1.0.  So half of our shots will lie in the inner half of the circle, 
	// and the other half on the outer half.  But since the area of the outer half is
	// larger than the inner half, we'll end up with a greater spread.
	// For a good writeup on randomness in circles (or lack of randomness), check
	// out http://mathworld.wolfram.com/DiskPointPicking.html
	// ---------------------------------------------------------------------------------

	// Convert to cartesian (X/Y) coordinates
	SinCos (theta, &sinTheta, &cosTheta);
	x = r * sinTheta;
	y = r * cosTheta;

	m_vecResult = m_vecShotDirection + x * vecSpread.x * m_vecRight + y * vecSpread.y * m_vecUp;

	return m_vecResult;
}


#endif // SHOT_MANIPULATOR_H
