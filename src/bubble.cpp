#include "bubble.hpp"
#include <stdio.h>
#include <cmath>
#include <ctime>


Bubble::Bubble(float r, float x, float y, float z) :
    m_radius(r),
    m_x(x),
    m_y(y),
    m_z(z),
    m_dx(0),
    m_dy(0),
    m_dz(0.1f),
    m_precision(50)
{
}

void Bubble::draw(int pass)
{
    glPushMatrix();
    glTranslatef(m_x, m_y, m_z);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.1f, 0.1f, 0.25f, 0.25f);
    //glColor4f(0.1f, 0.1f, 0.25f, 1.0f);
    glutSolidSphere(m_radius, m_precision, m_precision);
    glDisable(GL_BLEND);
    glPopMatrix();
}

void Bubble::animate()
{
    m_dx += ((random()%3)-1)*0.001f;
    m_dy += ((random()%3)-1)*0.001f;
    m_x += m_dx;
    m_y += m_dy;
    m_z += m_dz;
}
