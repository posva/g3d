#include "shark.hpp"
#include "objManager.hpp"
#include "const.hpp"

Shark::Shark() :
    m_body(objManager::getObj("shark")),
    m_teeth(objManager::getObj("shark_teeth")),
    m_eyes(objManager::getObj("shark_eyes")),
    m_chest(objManager::getObj("chest")),
    m_viewer(NULL),
    m_timer(0),
    m_pos(0, BEG_SHARK, COMMON_HEIGHT),
    m_rot(0),
    m_showChest(false)
{}

void Shark::draw(int pass)
{
    glPushMatrix();
    glTranslatef(m_pos.x, m_pos.y, m_pos.z);
    if (m_showChest) {
        glRotatef(-90, 1, 0, 0);
        glRotatef(180, 0, 0, 1);
        m_chest.draw(pass);
    } else {
        glRotatef(90, 1, 0, 0);
        glRotatef(m_rot, 1, 0, 0);
        m_body.draw(pass);
        m_teeth.draw(pass);
        m_eyes.draw(pass);
    }
    glPopMatrix();
}

void Shark::animate()
{
    if (m_timer > fps*4 && m_timer < fps*8) {
        //Le requin avance doucement
        m_pos.y -= vitesseLenteRequin;
    }
    if (m_timer > fps*11 && m_timer < fps*12) {
        //Le requin avance rapidement
        m_pos.y -= vitesseRapideRequin;
    }
    if (m_timer > fps*15 && m_timer < fps*16) {
        //Bisou
        m_pos.y -= distanceFaceAFace/fps;
    }
    if (m_timer > fps*16 && m_timer < fps*17) {
        //Bisou
        m_pos.y += distanceFaceAFace/fps;
    }
    if (m_timer > fps*18 && m_timer < fps*19) {
        //Le requin recule
        m_pos.y += distanceReculeRequin/fps;
    }
    if (m_timer > fps*19 && m_timer < fps*19.3) {
        //Le requin se tourne
        m_rot += rotationRequin/(fps*.3);
    }
    if (m_timer > fps*19.3 && m_timer < fps*19.6) {
        //Le requin s'enfonce
        m_pos.z -= (COMMON_HEIGHT-profondeurRequin)/(fps*.3);
    }
    if (m_timer > fps*24.2) {
        m_showChest = true;
        m_rot = 0;
        m_pos.z = 0;
    }
    m_timer++;
}
