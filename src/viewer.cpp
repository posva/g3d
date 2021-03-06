#include <QtGui/QKeyEvent>

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include "viewer.hpp"
#include "renderable.hpp"
#include "TextureManager.hpp"
#include "objManager.hpp"
#include "chest.hpp"
#include "submarine.hpp"
#include "stone.hpp"
#include "bubble.hpp"
#include "shark.hpp"
#include "glm/gtx/noise.hpp"
#include "skybox.hpp"
#include "weed.hpp"
#include "torse.hpp"
#include "dynamicSystem.hpp"
#include "cameraAnimation.hpp"
#include "coral.hpp"
#include "globals.hpp"
#include "const.hpp"
#include <sstream>
#include <ctime>

Viewer::Viewer() : currentCaustic(0), useCustomCamera(false), useCaustics(true), flock(NULL)
{
    lightDiffuseColor[0] = 0.66;
    lightDiffuseColor[1] = 1.0;
    lightDiffuseColor[2]= 0.66;
    lightDiffuseColor[3] = 1.0;
    lightPosition[0] = 0.0;
    lightPosition[1] = 10.0;
    lightPosition[2] = 30.0;
    lightPosition[3] = 1.0;
}

Viewer::~Viewer()
{
    list<Renderable *>::iterator it;
    for (it = renderableList.begin(); it != renderableList.end(); ++it) {
        delete(*it);
    }

    renderableList.clear();
    objManager::free();
    TextureManager::free();
}

void Viewer::addRenderable(Renderable *r)
{
    renderableList.push_back(r);
}

void Viewer::init()
{
    // glut initialisation (mandatory) 
    //int dum;
    //glutInit(&dum, NULL);
    // XXX WTF cet appel était en trop?

    srand(time(NULL));
    //=== VIEWING PARAMETERS
    restoreStateFromFile();   // Restore previous viewer state.

    toogleWireframe = false;  // filled faces
    toogleLight = true;       // light on
    help();                   // display help

    if (toogleLight)
        glEnable(GL_LIGHTING);
    else
        glDisable(GL_LIGHTING);


    // load textures & models
    glEnable(GL_TEXTURE_2D);
    loadTextures();

    glEnable(GL_NORMALIZE); // les nomrmales ne sont plus affectées par les scale

    // Création de la caméra et animation
    CameraAnimation &cam = *(new CameraAnimation(30*100, *camera(), useCustomCamera));

    cam.addFrame(0, glm::vec3(-20, -BEG_DIST+10, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST, COMMON_HEIGHT), true);
    cam.addFrame(30, glm::vec3(0, -BEG_DIST+20+30*SWIM_SPD, COMMON_HEIGHT+10), glm::vec3(0, -BEG_DIST+SWIM_SPD*30, COMMON_HEIGHT), true);
    cam.addFrame(60, glm::vec3(20, -BEG_DIST+10+60*SWIM_SPD, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+60*SWIM_SPD, COMMON_HEIGHT), true);
    cam.addFrame(62, glm::vec3(0, 0, 50), glm::vec3(0, 0, 0), false);
    cam.addFrame(120, glm::vec3(10, BEG_SHARK, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+120*SWIM_SPD+20, COMMON_HEIGHT), false);
    cam.addFrame(190, glm::vec3(10, BEG_SHARK, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+120*SWIM_SPD+20, COMMON_HEIGHT), false);
    cam.addFrame(320, glm::vec3(-30, -BEG_DIST+180*SWIM_SPD, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+180*SWIM_SPD+10, COMMON_HEIGHT), true);
    cam.addFrame(570, glm::vec3(-30, -BEG_DIST+180*SWIM_SPD, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+180*SWIM_SPD+10, 0), false);
    cam.addFrame(630, glm::vec3(-30, -BEG_DIST+180*SWIM_SPD, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+180*SWIM_SPD+10, 0), false);
    cam.addFrame(700, glm::vec3(0, -BEG_DIST+140*SWIM_SPD, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+180*SWIM_SPD+20, COMMON_HEIGHT), true);
    cam.addFrame(800, glm::vec3(0, -BEG_DIST+140*SWIM_SPD, COMMON_HEIGHT), glm::vec3(0, -BEG_DIST+180*SWIM_SPD+20, COMMON_HEIGHT), false);
    cam.addFrame(1000, glm::vec3(0, -BEG_DIST+140*SWIM_SPD, 7), glm::vec3(0, -BEG_DIST+150*SWIM_SPD+20, 8), true);

    cam.interpolate();

    addRenderable(&cam);

    // begin with the skybox
    addRenderable(new Skybox());

    noise = new NoiseTerrain();
    noise_zoom = 50;
    noise_persistence = 0.95;
    noise_octaves = 13;

    noise->generateClouds(100, 100, noise_zoom, noise_persistence, noise_octaves);
    addRenderable(noise);

    //Corals
    float coralOffsetX=0;
    float coralOffsetY=0;
    int i=0;
    for (i=0; i < 25; i++) {
        coralOffsetX = glm::simplex(glm::vec3(coralOffsetX*3, coralOffsetY ,1.0))*10;
        coralOffsetY = glm::simplex(glm::vec3(coralOffsetX, coralOffsetY*10 ,1.0))*10;
        addRenderable(new Coral(Coral::defaultDepth, coralOffsetX, coralOffsetY,
                    Coral::randomBetween(Coral::minMult,Coral::maxMult),
                    noise->getZ(coralOffsetX, coralOffsetY)));
    }
    for (i=0; i < 20; i++) {
        coralOffsetX = (glm::simplex(glm::vec3(coralOffsetX*3, coralOffsetY ,1.0))+3)*10;
        coralOffsetY = (glm::simplex(glm::vec3(coralOffsetX, coralOffsetY*10 ,1.0))+3)*10;
        addRenderable(new Coral(Coral::defaultDepth, coralOffsetX+3, coralOffsetY,
                    Coral::randomBetween(Coral::minMult,Coral::maxMult),
                    noise->getZ(coralOffsetX, coralOffsetY)));

    }

    //addRenderable(new objReader("models/cat.obj", "gfx/cat.png"));
    //addRenderable(new objReader("models/rpg.obj", "gfx/rpg.jpg"));
    //addRenderable(new objReader("models/missile.obj", "gfx/missile.jpg"));
    //addRenderable(new objReader("models/portalbutton.obj", "gfx/button.jpg"));
    //addRenderable(new objReader("models/TropicalFish01.obj", "gfx/fishes/TropicalFish01.jpg"));
    //addRenderable(new Chest());
    addRenderable(new Submarine());
    addRenderable(new Shark());
    guy = new Torse();
    addRenderable(guy);
    flock = new Flock(env, "fish");
    addRenderable(flock);

    //Useless XXX
    //for (int32_t y = -TERRAIN_HEIGHT/2; y < TERRAIN_HEIGHT/2; ++y) {
    //for (int32_t x = -TERRAIN_WIDTH/2; x < TERRAIN_WIDTH/2; x++) {
    //float nn = glm::simplex(glm::vec2(x, y));
    //if (nn > 0.78) {
    //Stone *s = new Stone();
    //s->m_size = nn*10.f;
    //s->m_pos.x = x;
    //s->m_pos.y = y;
    //s->m_pos.z = 0.2; // TODO get z from the terrain
    //addRenderable(s);
    //}
    //}
    //}

    list<Renderable *>::iterator it;
    for (it = renderableList.begin(); it != renderableList.end(); ++it) {
        (*it)->init(*this);
    }


    glDisable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
    glDisable(GL_LIGHT4);
    glDisable(GL_LIGHT5);
    glDisable(GL_LIGHT6);
    glDisable(GL_LIGHT7);

    // fish light
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, .0);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.9);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiffuseColor);

    // ambient light
    glLightfv(GL_LIGHT2, GL_AMBIENT, lightDiffuseColor);
    //glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, .0);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.9);
    //glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.5);

    //diver light
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT3, GL_AMBIENT,  white);
    glLightfv(GL_LIGHT3, GL_DIFFUSE,  white);
    glLightfv(GL_LIGHT3, GL_SPECULAR, white);
    glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 3.0f);
    glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 1.0f);


    // fog
    fogColor[0] = 0.333;
    fogColor[1] = 0.5;
    fogColor[2] = 0.5;
    fogColor[3] = 1.0;

    GLfloat density = 0.0040;
    glEnable (GL_FOG);
    glFogi (GL_FOG_MODE, GL_EXP2);
    glFogfv (GL_FOG_COLOR, fogColor);
    glFogf (GL_FOG_DENSITY, density);
    glHint (GL_FOG_HINT, GL_NICEST);

    //addRenderable(new Weed());

}

void Viewer::loadTextures()
{
    TextureManager::loadTexture("gfx/sand1.jpg", "sand1");
    TextureManager::loadTexture("gfx/corail1.jpg", "corail1");
    TextureManager::loadTexture("gfx/weed.png", "weed");

    std::stringstream file, key;
    for (uint32_t i = 0; i < NUM_PATTERNS; ++i) {
        file.str("");
        key.str("");
        key<<"caust"<<i;
        file<<"gfx/caustics/caust"<<i<<".jpg";
        causticsTex[i] = TextureManager::loadTextureMipmaps(file.str().c_str(), key.str());
    }

    //Skybox
    //Pour garder les noms de fichiers explicites et pas des "i", on charge à la main
    TextureManager::loadTexture("gfx/skybox/back.jpg", "sky_back");
    TextureManager::loadTexture("gfx/skybox/front.jpg", "sky_front");
    TextureManager::loadTexture("gfx/skybox/left.jpg", "sky_left");
    TextureManager::loadTexture("gfx/skybox/right.jpg", "sky_right");
    TextureManager::loadTexture("gfx/skybox/top.jpg", "sky_top");


    // obj
    objManager::loadObj("models/TropicalFish.obj", "gfx/TropicalFish.jpg", "fish");
    objManager::loadObj("models/submarine.obj", "gfx/submarine.jpg", "submarine");
    objManager::loadObj("models/rpg.obj", "gfx/rpg.jpg", "rpg");
    objManager::loadObj("models/missile.obj", "gfx/missile.jpg", "missile");
    objManager::loadObj("models/treasure_chest.obj", "gfx/treasure_chest.jpg", "chest");
    objManager::loadObj("models/shark.obj", "gfx/shark.jpg", "shark");
    objManager::loadObj("models/shark_eyes.obj", "gfx/shark_eyes.jpg", "shark_eyes");
    objManager::loadObj("models/shark_teeth.obj", "gfx/shark_teeth.jpg", "shark_teeth");
}


void Viewer::draw()
{  
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // greenish light for the ambient
    glLightfv(GL_LIGHT2, GL_POSITION, lightPosition);

    //GLfloat fishLight[4];
    //if (flock) {
    //Fish *f = flock->getLeader();
    //fishLight[0] = f->getPos()[0];
    //fishLight[1] = f->getPos()[1];
    //fishLight[2] = f->getPos()[2];
    //fishLight[3] = 1.f;
    //glLightfv(GL_LIGHT1, GL_POSITION, fishLight);
    //}

    glm::vec3 pos = guy->getHeadPos(),
        dir = guy->getLookAt();
    GLfloat positionL3[4]= {pos.x, pos.y, pos.z, 1.0};
    glLightfv(GL_LIGHT3, GL_POSITION, positionL3);
    GLfloat light3_direction[] = {dir.x, dir.y, dir.z, 1.f};
    //std::cout<<"dir:"<<dir.x<<","<<dir.y<<","<<dir.z<<"\n";
    //std::cout<<"pos:"<<pos.x<<","<<pos.y<<","<<pos.z<<"\n";

    glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, light3_direction);



    // === FIRST PASS NORMAL ===
    //glDisable(GL_TEXTURE_2D);

    // draw every objects in renderableList
    list<Renderable *>::iterator it;
    for(it = renderableList.begin(); it != renderableList.end(); ++it) {
        (*it)->draw(PASS_NORMAL);
    }

    if (useCaustics) {
        // === SECOND PASS CAUSTICS :3 ===
        /* Disable depth buffer update and exactly match depth
           buffer values for slightly faster rendering. */
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_EQUAL);

        /* Multiply the source color (from the caustic luminance
           texture) with the previous color from the normal pass.  The
           caustics are modulated into the scene. */
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);

        GLfloat sPlane[4] = { 0.05, 0.03, 0.0, 0.0 };
        GLfloat tPlane[4] = { 0.0, 0.03, 0.05, 0.0 };
        GLfloat causticScale = 1.0;

        /* The causticScale determines how large the caustic "ripples" will
           be.  See the "Increate/Decrease ripple size" menu options. */

        sPlane[0] = 0.05 * causticScale;
        sPlane[1] = 0.03 * causticScale;

        tPlane[1] = 0.03 * causticScale;
        tPlane[2] = 0.05 * causticScale;

        /* Set current color to "white" and disable lighting
           to emulate OpenGL 1.1's GL_REPLACE texture environment. */
        glColor3f(1.0, 1.0, 1.0);
        glDisable(GL_LIGHTING);

        /* Generate the S & T coordinates for the caustic textures
           from the object coordinates. */

        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGenfv(GL_S, GL_OBJECT_PLANE, sPlane);
        glTexGenfv(GL_T, GL_OBJECT_PLANE, tPlane);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);

        glBindTexture(GL_TEXTURE_2D, causticsTex[currentCaustic]);

        // draw every objects in renderableList
        for(it = renderableList.begin(); it != renderableList.end(); ++it) {
            (*it)->draw(PASS_CAUSTIC);
        }
        if (toogleLight)
            glEnable(GL_LIGHTING);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);

        /* Restore fragment operations to normal. */
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
    }

}


void Viewer::animate()
{
    currentCaustic = (currentCaustic + 1) % NUM_PATTERNS;
    // animate every objects in renderableList
    list<Renderable *>::iterator it;
    for(it = renderableList.begin(); it != renderableList.end(); ++it) {
        (*it)->animate();
    }

    // this code might change if some rendered objets (stored as
    // attributes) need to be specifically updated with common
    // attributes, like real CPU time (?)
}


void Viewer::mouseMoveEvent(QMouseEvent *e)
{
    // all renderables may respond to key events
    list<Renderable *>::iterator it;
    for(it = renderableList.begin(); it != renderableList.end(); ++it) {
        (*it)->mouseMoveEvent(e, *this);
    }

    // default QGLViewer behaviour
    QGLViewer::mouseMoveEvent(e);
    updateGL();
}

void Viewer::keyPressEvent(QKeyEvent *e)
{
    // Get event modifiers key
    const Qt::KeyboardModifiers modifiers = e->modifiers();

    // all renderables may respond to key events
    list<Renderable *>::iterator it;
    for(it = renderableList.begin(); it != renderableList.end(); ++it) {
        (*it)->keyPressEvent(e, *this);
    }
    int s = 100;
    if ((e->key()==Qt::Key_W) && (modifiers==Qt::NoButton)) {
        // events with modifiers: CTRL+W, ALT+W, ... to handle separately
        toogleWireframe = !toogleWireframe;
        if (toogleWireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else if ((e->key()==Qt::Key_L) && (modifiers==Qt::NoButton)) {
        toogleLight = !toogleLight;
        if (toogleLight == true)
            glEnable(GL_LIGHTING);
        else
            glDisable(GL_LIGHTING);       
        // ... and so on with all events to handle here!
    } else if (e->key() == Qt::Key_H) {
        noise_zoom += modifiers==Qt::NoButton?-1.0:1.0;
        std::cout<<"zoom:"<<noise_zoom<<"\n";
        noise->generateClouds(s, s, noise_zoom, noise_persistence, noise_octaves);
    } else if (e->key() == Qt::Key_K) {
        noise_persistence += modifiers==Qt::NoButton?-0.05:0.05;
        std::cout<<"noise_persistence:"<<noise_persistence<<"\n";
        noise->generateClouds(s, s, noise_zoom, noise_persistence, noise_octaves);
    } else if (e->key() == Qt::Key_J) {
        noise_octaves += modifiers==Qt::NoButton?-1:1;
        std::cout<<"noise_octaves:"<<noise_octaves<<"\n";
        noise->generateClouds(s, s, noise_zoom, noise_persistence, noise_octaves);
    } else if (e->key() == Qt::Key_C) {
        useCustomCamera = !useCustomCamera;
    } else if (e->key() == Qt::Key_X) {
        useCaustics = !useCaustics;
    } else {
        // if the event is not handled here, process it as default
        QGLViewer::keyPressEvent(e);
    }
    updateGL();
}


QString Viewer::helpString() const
{
    // Some usefull hints...
    QString text("<h2>V i e w e r</h2>");
    text += "Use the mouse to move the camera around the object. ";
    text += "You can respectively revolve around, zoom and translate with the three mouse buttons. ";
    text += "Left and middle buttons pressed together rotate around the camera view direction axis<br><br>";
    text += "Pressing <b>Alt</b> and one of the function keys (<b>F1</b>..<b>F12</b>) defines a camera keyFrame. ";
    text += "Simply press the function key again to restore it. Several keyFrames define a ";
    text += "camera path. Paths are saved when you quit the application and restored at next start.<br><br>";
    text += "Press <b>F</b> to display the frame rate, <b>A</b> for the world axis, ";
    text += "<b>Alt+Return</b> for full screen mode and <b>Control+S</b> to save a snapshot. ";
    text += "See the <b>Keyboard</b> tab in this window for a complete shortcut list.<br><br>";
    text += "Double clicks automates single click actions: A left button double click aligns the closer axis with the camera (if close enough). ";
    text += "A middle button double click fits the zoom of the camera and the right button re-centers the scene.<br><br>";
    text += "A left button double click while holding right button pressed defines the camera <i>Revolve Around Point</i>. ";
    text += "See the <b>Mouse</b> tab and the documentation web pages for details.<br><br>";
    text += "Press <b>Escape</b> to exit the viewer.";
    return text;
}

