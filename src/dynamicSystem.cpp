#include <cmath>
#include <iostream>
#include <map>
using namespace std;

#include "viewer.hpp"
#include "dynamicSystem.hpp"


DynamicSystem::DynamicSystem()
	: 
	defaultGravity(0.0, 0.0, -10.0),
	defaultMediumViscosity(1.0),
	dt(0.1),
	groundPosition(0.0, 0.0, 0.0),
	groundNormal(0.0, 0.0, 1.0),
	rebound(0.5)
{
	// default values reset in init()
}

DynamicSystem::~DynamicSystem()
{
	clear();
}


void DynamicSystem::clear()
{
	vector<Particle *>::iterator itP;
	for (itP = particles.begin(); itP != particles.end(); ++itP) {
 		delete(*itP);
	}
	particles.clear();

	vector<Spring *>::iterator itS;
	for (itS = springs.begin(); itS != springs.end(); ++itS) {
 		delete(*itS);
	}
	springs.clear();
}

const Vec &DynamicSystem::getFixedParticlePosition() const
{
	return particles[0]->getPosition();	// no check on 0!
}

void DynamicSystem::setBeginingPosition(const Vec &pos)
{
	if (particles.size() > 0)
		particles[0]->setPosition(pos);
}

void DynamicSystem::setEndPosition(const Vec &pos)
{
	if (particles.size() > 0)
		particles[springs.size()]->setPosition(pos);
}

void DynamicSystem::setEndParticlePosition(const Vec &pos)
{
	if (particles.size() > 0)
		particles[particles.size()-1]->setPosition(pos);
}

void DynamicSystem::setGravity(bool onOff)
{
	gravity = (onOff ? defaultGravity : Vec());
}

void DynamicSystem::setViscosity(bool onOff)
{
	mediumViscosity = (onOff ? defaultMediumViscosity : 0.0);
}

void DynamicSystem::setCollisionsDetection(bool onOff)
{
	handleCollisions = onOff;
}


///////////////////////////////////////////////////////////////////////////////
void DynamicSystem::init(Vec v)
{
	toggleGravity = true;
	toggleViscosity = true;
	toggleCollisions = true;
	clear();
	
	// global scene parameters 
	defaultGravity = Vec(0.0, 0.0, -10.0);
	gravity = defaultGravity;
	defaultMediumViscosity = 1.0;
	mediumViscosity = defaultMediumViscosity;
	handleCollisions = true;
	dt = 0.1;
	groundPosition = Vec(0.0, 0.0, 0.0);
	groundNormal = Vec(0.0, 0.0, 1.0);
	rebound = 0.5;	
	// parameters shared by all particles
	particleMass = 1.0;
	particleRadius = 0.25;
	distanceBetweenParticles = 4.0 * particleRadius;
	// parameters shared by all springs
	springStiffness = 30.0;
	springInitLength = 0.5;
	springDamping = 1.0;

	createSystemScene(v);
	// or another method, e.g. to test collisions on simple cases...
// 	createTestCollisions();

	// add a manipulatedFrame to move particle 0 with the mouse
/*        viewer.setManipulatedFrame(new qglviewer::ManipulatedFrame());
	viewer.manipulatedFrame()->setPosition(getFixedParticlePosition());
	viewer.setSceneRadius(10.0f);*/

}


///////////////////////////////////////////////////////////////////////////////
void DynamicSystem::createSystemScene(Vec v)
{
	// add a fixed particle
	Vec initPos = v;
	particles.push_back(new Particle(initPos, Vec(), 0.0, particleRadius));

    int nParts(5);

    Vec pos, prevPos(initPos),
        l0,
        vel(0.0, 0.0, 0.0);
    for (int i = 0; i < nParts; ++i) {
        pos = initPos + Vec(0.0, -distanceBetweenParticles*(i+1), 0.0);
        particles.push_back(new Particle(pos, vel, i==nParts-1?0.0:particleMass, particleRadius));
        l0 = pos - prevPos;

        Spring *spr = new Spring(particles[i], particles[i+1], springStiffness, l0.norm(), springDamping);
        springs.push_back(spr);

        prevPos = pos;
    }
    particles.push_back(new Particle(Vec(v), Vec(0.0, 0.0, 0.0), 0, particleRadius*5.0));
    particles.back()->blue = true;

}


///////////////////////////////////////////////////////////////////////////////
void DynamicSystem::draw(int pass)
{
	// Particles
	glColor3f(1,0,0);
	vector<Particle *>::iterator itP;
	for (itP = particles.begin(); itP != particles.end(); ++itP) {
        if(itP+1 != particles.end())
    		(*itP)->draw(pass);
	}

	// Springs
	glColor3f(1.0, 0.28, 0.0);
	glLineWidth(5.0);
	vector<Spring *>::iterator itS;
    int nb = 0;
	for (itS = springs.begin(); itS != springs.end(); ++itS) {
    	(*itS)->draw(pass);
	}
	glLineWidth(1.0);
}


///////////////////////////////////////////////////////////////////////////////
void DynamicSystem::animate()
{

//======== 1. Compute all forces
	// map to accumulate the forces to apply on each particle
	map<const Particle *, Vec> forces;

	// weights
	vector<Particle *>::iterator itP;
	for (itP = particles.begin(); itP != particles.end(); ++itP) {
		Particle *p = *itP;
		forces[p] = gravity * p->getMass();
        forces[p] += -mediumViscosity * p->getVelocity();
	}

	// damped springs
	vector<Spring *>::iterator itS;
	for (itS = springs.begin(); itS != springs.end(); ++itS) {
		Spring *s = *itS;
		Vec f12 = s->getCurrentForce();
		forces[s->getParticle1()] += f12;
		forces[s->getParticle2()] -= f12; // opposite force
	}


//======== 2. Integration scheme

	// update particles speed
	for (itP = particles.begin(); itP != particles.end(); ++itP) {
		Particle *p = *itP;
        // v = v + dt * a/m
		p->incrVelocity(dt * (forces[p] * p->getInvMass()));
	}

	// update particles positions
	for (itP = particles.begin(); itP != particles.end(); ++itP) {
		Particle *p = *itP;
		// q = q + dt * v
        // v = v + dt * a/m
		p->incrPosition(dt * p->getVelocity());
	}


//======== 3. Collisions
	if (handleCollisions) {
		//TO DO: discuss multi-collisions and order!
                // XXX Not neede here
		//for (itP = particles.begin(); itP != particles.end(); ++itP) {
			//collisionParticleGround(*itP);
		//}	
		for(unsigned int i = 0; i < particles.size(); ++i) {
			for(unsigned int j = 1; j < particles.size(); ++j) {
				if ( i != j) {
					Particle *p1 = particles[j]; 
					Particle *p2 = particles[i];
	            	collisionParticleParticle(p1, p2);
	            }
        	}
		}
	}
}



void DynamicSystem::collisionParticleGround(Particle *p)
{
	// don't process fixed particles (ground plane is fixed)
	if (p->getInvMass() == 0)
		return;

	// particle-plane distance
	double penetration = (p->getPosition() - groundPosition) * groundNormal;
	penetration -= p->getRadius();
	if (penetration >= 0)
		return;

	// penetration velocity
	double vPen = p->getVelocity() * groundNormal;

	// updates position and velocity of the particle
	p->incrPosition(-penetration * groundNormal);
	p->incrVelocity(-(1 + rebound) * vPen * groundNormal);
}


void DynamicSystem::collisionParticleParticle(Particle *p1, Particle *p2)
{
	if (p1->getInvMass() == 0)
    return;

    Vec p(p2->getPosition() - p1->getPosition());

    double penetration = p.norm() - p1->getRadius() - p2->getRadius();

    if (penetration >= 0.0)
        return;

    p.normalize();

    //std::cout<<"Collision\n";

    Vec vel(p1->getVelocity() - p2->getVelocity());

    p1->incrPosition(penetration*p*0.5);
    //p1->incrVelocity(-p1->getVelocity()*p1->getMass()/(p1->getMass()+p2->getMass()));
    p1->incrVelocity(-p2->getMass()/(p1->getMass()+p2->getMass())
                    *(p*vel)*(1.0+rebound)*p);

    if (p2->getInvMass() == 0.0)
        return;
    p *= -1.0;
    p2->incrPosition(penetration*p*0.5);
    //p2->incrVelocity(-p2->getVelocity()*p2->getMass()/(p1->getMass()+p2->getMass()));
    p2->incrVelocity(p1->getMass()/(p1->getMass()+p2->getMass())
                   *(p*vel)*(1.0+rebound)*p);
}


void DynamicSystem::keyPressEvent(QKeyEvent* e, Viewer& viewer)
{
  	// Get event modifiers key
	const Qt::KeyboardModifiers modifiers = e->modifiers();

        /* Controls added for Lab Session 6 "Physicall Modeling" */
        if ((e->key()==Qt::Key_G) && (modifiers==Qt::NoButton)) {
		toggleGravity = !toggleGravity;
		setGravity(toggleGravity);
		viewer.displayMessage("Set gravity to "
			+ (toggleGravity ? QString("true") : QString("false")));
	
	} else if ((e->key()==Qt::Key_V) && (modifiers==Qt::NoButton)) {
		toggleViscosity = !toggleViscosity;
		setViscosity(toggleViscosity);
		viewer.displayMessage("Set viscosity to "
			+ (toggleViscosity ? QString("true") : QString("false")));

	} else if ((e->key()==Qt::Key_C) && (modifiers==Qt::NoButton)) {
		toggleCollisions = !toggleCollisions;
		setCollisionsDetection(toggleCollisions);
		viewer.displayMessage("Detects collisions "
			+ (toggleCollisions ? QString("true") : QString("false")));

	} else if ((e->key()==Qt::Key_Home) && (modifiers==Qt::NoButton)) {
		// stop the animation, and reinit the scene
		viewer.stopAnimation();
		//init();
		viewer.manipulatedFrame()->setPosition(getFixedParticlePosition());
		toggleGravity = true;
		toggleViscosity = true;
		toggleCollisions = true;
	}
}	

