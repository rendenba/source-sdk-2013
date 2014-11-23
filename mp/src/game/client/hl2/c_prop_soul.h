//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CPROPSOUL_H_
#define CPROPSOUL_H_

#ifdef _WIN32
#pragma once
#endif

#include "c_prop_combine_ball.h"

class C_PropSoul : public C_PropCombineBall
{
	DECLARE_CLASS( C_PropSoul, C_PropCombineBall );
	DECLARE_CLIENTCLASS();
public:

	C_PropSoul( void );

protected:

};


#endif