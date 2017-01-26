#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include <string>
#include <math.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <string>

#include "Helper.hpp"
#include "Physics.hpp"
#include "Thing.hpp"
#include "World.hpp"

Physics::Physics(World* the_world) : world(the_world) {
}

Derivative Physics::evaluate( const std::string name, 
        double t, 
        double dt, 
        const Derivative& d )
{
	Thing* initial = world->object(name);
    // advance position/velocity by their change*change in time
	// need here to make deep copy of object
	Thing state = Thing(*initial);
	state.position += d.dx*(float)dt;
	state.momentum += d.dp*(float)dt;
    // new change in position becomes new velocity
    // new change in velo becomes new state change with respect to time
	//std::cout << "Dickbutt " << printVec(state.velocity()) << "\n";
    Derivative output;
    output.dx = state.velocity();
    output.dp = world_force(state, dt);
	//std::cout << "output dx and dp vectors " << printVec(output.dx) << " " << printVec(output.dp) << "\n";
    return output;
}

// currently hooke's law/spring constant, but have access to
// position and velocity here

glm::vec3 Physics::world_force( Thing& state, double dt )
{
    glm::vec3 force = glm::vec3(0.0f,0.0f,0.0f);
	glm::vec3 f = state.net_force() * (float)dt;
	force += f;
	//std::cout << "Start force on obj " << printVec(force) << "\n";

    // air resistance
    // proportional to square of velocity of object

    bool air = true;
	glm::vec3 airRes = glm::vec3(0.0f,0.0f,0.0f);
    if (air) {
		const float air_decel = -0.25f;
        airRes.x += air_decel*state.momentum.x;
        airRes.y += air_decel*state.momentum.y;
        airRes.z += air_decel*state.momentum.z;
		airRes *= (float)dt;
    }
	force += airRes;
	//std::cout << "Force after air " << printVec(force) << "\n";
    
	bool decelF = false;
    // this is a force to stop the movement really, so don't get the sliding forever
	if (decelF) {
		float minForce = 5.0f*std::fabs(state.mass);
		float decel = 1.0f;
		force.x += -signOf(state.velocity().x) *
			std::min(minForce,decel*std::fabs(state.momentum.x));  
		force.y += -signOf(state.velocity().y) *
			std::min(minForce,decel*std::fabs(state.momentum.y));  
		force.z += -signOf(state.velocity().z) *
			std::min(minForce,decel*std::fabs(state.momentum.z));  
	}
    // gravity - in my program, z is up
    // will implement later could also do friction, but not now
	//std::cout << "Net force on object " << printVec(force) << "\n";
    
    return force;
}

void Physics::integrate( const std::string objectName, 
        double t, 
        double dt )
{
	t /= 10.0;
	dt /= 10.0;
    Derivative a,b,c,d;

    // essentially calc new pos/velo at time t with dt at 0
    // then on that go again with half dt, repeat this
    // then find for full dt change
    // weighted average using Taylor series
	// these don't change the statec
    a = evaluate( objectName, t, 0.0f, Derivative() );
    b = evaluate( objectName, t, dt*0.5f, a );
    c = evaluate( objectName, t, dt*0.5f, b );
    d = evaluate( objectName, t, dt, c );

    glm::vec3 dxdt = 1.0f / 6.0f * 
        ( a.dx + 2.0f*(b.dx + c.dx) + d.dx );

    glm::vec3 dpdt = 1.0f / 6.0f * 
        ( a.dp + 2.0f*(b.dp + c.dp) + d.dp );

	Thing* state = world->object(objectName);
    state->position = state->position + dxdt * (float)dt;
    state->momentum = state->momentum + dpdt * (float)dt;
	state->net_force(); // collapses force vector to nothing
}

/*
int the_main() {
    bool quit = false;
    //std::cout << "hi" << "\n";
    State currentState = State
        (glm::vec3(0.0f,0.0f,0.0f), // pos
         glm::vec3(50.0f,-50.0f,5.0f), // velocity
         10.0f
        );
	Thing thing = Thing(
		glm::vec3(2.0f,2.0f,2.0f), //pos
		glm::vec3(-1.0f,-1.0f,-1.0f), //lookingAt
		glm::vec3(0.5f,0.0f,0.0f), //velocity
		glm::vec3(0.02f,0.02f,0.02f), //accel, that will be applied
		"Thing",
		1.0f
		);

    double simsPerSec = 60.0;
    double fps = 60.0;

    double t = 0.0;
    double dt = 1000.0/simsPerSec;

    const double startTime = time_now();
    double currentTime = time_now();
    double accumulator = 0.0;
    int i = 0;
    int r = 0;

    //State previous;
    //Thing current;

    while ( !quit )
    {
        double newTime = time_now();
        
        // ms
        double frameTime = (newTime - currentTime)/1000000.0f;
        // won't work below 4fps really
        //std::cout << "Frametime " << frameTime << "ms" << "\n";
        currentTime = newTime;

        // add frameTime into acc
        accumulator += frameTime;
        //std::cout << "Acc start " << accumulator << "\n";
        // 
        while ( accumulator >= dt )
        {
            //previousState = currentState;
            //std::cout << "State " << ++i << " " << (time_now()-startTime)/1e9 << "s" << "\n";
            integrate( thing, t, dt );
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            t += dt;
            accumulator -= dt;
        }

        //std::cout << "Render " << ++r << " and acc " << accumulator << " " << (time_now()-startTime)/1e9 << "s" << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        //const double alpha = accumulator / dt;

        //State state = currentState * alpha + 
        //  previousState * ( 1.0 - alpha );

        //render( state );
        
        // my code to limit this to 60fps
        const double loopMaxTime = 1000000000.0/fps;
        double nsThisLoop = time_now() - newTime;

        long nanoSleep = static_cast<long>(loopMaxTime - nsThisLoop);
        nanoSleep = std::max(0l,nanoSleep);

        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSleep));
        //std::cout << "\n";
    }
    return 0;
}
*/
