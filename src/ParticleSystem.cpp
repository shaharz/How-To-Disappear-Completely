/*
 *  ParticleSystem.cpp
 *
 *  Created by Mehmet Akten on 02/05/2009.
 *  Copyright 2009 MSA Visuals Ltd.. All rights reserved.
 *
 */

#include "ParticleSystem.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/app/App.h"

using namespace ci;

ParticleSystem::ParticleSystem() 
{
	curIndex = 0;
	setWindowSize( Vec2i( 1, 1 ) );
}

void ParticleSystem::setWindowSize( Vec2i winSize )
{
	windowSize = winSize;
	invWindowSize = Vec2f( 1.0f / winSize.x, 1.0f / winSize.y );
}

void ParticleSystem::reset() {
    for(int i=0; i<MAX_PARTICLES; i++) {
        particles[i].dead = true;
    }
    
    curIndex = 0;
}

void ParticleSystem::updateAndDraw(){
	glEnable(GL_BLEND);
	glDisable( GL_TEXTURE_2D );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	for(int i=0; i<MAX_PARTICLES; i++) {
		if(!particles[i].dead) {
			particles[i].update( *solver, windowSize, invWindowSize );
			particles[i].updateVertexArrays( i, posArray, colArray);
		}
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, posArray);
	
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(3, GL_FLOAT, 0, colArray);
	
	glDrawArrays(GL_POINTS, 0, MAX_PARTICLES);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
    
	glDisable(GL_BLEND);
}

void ParticleSystem::addParticle( const Vec2f &pos, float c ) {
	particles[curIndex].init( pos.x, pos.y, c);
	curIndex++;
	if(curIndex >= MAX_PARTICLES) curIndex = 0;
}
