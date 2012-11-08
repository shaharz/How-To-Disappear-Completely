/*
 *  Particle.h
 *
 *  Created by Mehmet Akten on 02/05/2009.
 *  Copyright 2009 MSA Visuals Ltd.. All rights reserved.
 *
 */

#pragma once

#include "ciMsaFluidSolver.h"
#include "cinder/Vector.h"

class Particle {
  public:	
    ci::Vec2f	pos, vel;
    float       col;
    float		mass;
    
    bool        dead;
	
    void init(float x, float y, float col);
    void update( const ciMsaFluidSolver &solver, const ci::Vec2f &windowSize, const ci::Vec2f &invWindowSize );
	void updateVertexArrays( int i, float* posBuffer, float* colBuffer);
};

