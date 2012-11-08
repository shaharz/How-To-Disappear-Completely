/*
 *  Particle.cpp
 *
 *  Created by Mehmet Akten on 02/05/2009.
 *  Copyright 2009 MSA Visuals Ltd.. All rights reserved.
 *
 */

#include "Particle.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"

static const float MOMENTUM = 0.5f;
static const float FLUID_FORCE = 0.6f;

using namespace ci;

void Particle::init( float x, float y, float c ) {
	pos = Vec2f( x, y );
	vel.x = vel.y = 0;
	dead = false;
    col = c;
	mass = Rand::randFloat( 0.3f, 0.5f );
}

void Particle::update( const ciMsaFluidSolver &solver, const Vec2f &windowSize, const Vec2f &invWindowSize ) {
	// only update if particle is visible
	if( dead )
		return;
	
	vel = solver.getVelocityAtPos( pos * invWindowSize ) * (mass * FLUID_FORCE ) * windowSize + vel * MOMENTUM;
	pos += vel;	

	if( pos.x < 0
       || pos.x > windowSize.x
       || pos.y < 0
       || pos.y > windowSize.y ) {
        dead = true;
	}
	
//	// fade out a bit (and kill if alpha == 0);
//	alpha *= 0.999f;
//	if( alpha < 0.01f )
//		alpha = 0;
}


void Particle::updateVertexArrays( int i, float* posBuffer, float* colBuffer) {
	int vi = i * 2;
	posBuffer[vi++] = pos.x;
	posBuffer[vi++] = pos.y;
	
	int ci = i * 3;
    colBuffer[ci++] = col;
    colBuffer[ci++] = col;
    colBuffer[ci++] = col;
}
