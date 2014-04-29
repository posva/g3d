#include <iostream>
using namespace std;
#include "coral.hpp"

Coral::Coral(int depth) : 
	m_coral(depth,depth*0.3,25)
{
	m_depth=depth;
	m_nbBranch = 2;
	m_depthBranch = depth;

	//Recursive
	if (depth > 0) {
		cout << "Recursive " << depth << endl;
		for (int i=0; i < m_nbBranch; i++) {
			cout << "Pushing coral " << i << endl;
			m_smallCorals.push_back(Coral(depth-1));
		}
	}
}

void Coral::draw()
{
    glPushMatrix();
	glTranslatef(4,0.2,0.2);
	m_coral.draw();

	vector<Coral>::iterator it;
	int i=0;
	for (it=m_smallCorals.begin(); it != m_smallCorals.end(); it++) {
		if (i%2 == 0) {
		    it->drawSub(-defaultAngle);
		} else {
		    it->drawSub(defaultAngle);
		}			
		i++;
	}
    glPopMatrix();
}

void Coral::drawSub(int angle)
{
    glPushMatrix();
	//Decale les branches vers le haut
	glTranslatef(0,0, m_coral.getHeigth() - m_depth/1.7);
	//Pivote pour détacher le nouveau cylindre
	glRotatef(angle,1,0,0);
	//Pivote pour eviter d'aligner toutes les branches dans le même plan
	glRotatef(angle*2,0,0,1);
	m_coral.draw();

	vector<Coral>::iterator it;
	int i=0;
	for (it=m_smallCorals.begin(); it != m_smallCorals.end(); it++) {
		if (i%2 == 0) {
		    it->drawSub(-defaultAngle);
		} else {
		    it->drawSub(defaultAngle);
		}			
		i++;
	}
    glPopMatrix();

}