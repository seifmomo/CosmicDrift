#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>

// ============================================================
// CONSTANTS
// ============================================================
static const int WINDOW_W = 1024;
static const int WINDOW_H = 768;
static const int MAX_STARS = 650;
static const int MAX_ASTEROIDS = 30;
static const int MAX_RINGS = 15;
static const int MAX_PARTICLES = 1500;
static const int RING_SEGMENTS = 32;

static const float PI = 3.14159265358979f;
static const float DEG = PI / 180.0f;

static const float STEER_SPEED = 12.0f;
static const float SHIP_SPEED_START = 20.0f;
static const float SHIP_SPEED_MAX = 45.0f;
static const float SPEED_INCREASE = 0.3f;
static const float BOOST_ADD = 15.0f;
static const float BOOST_FUEL_MAX = 100.0f;
static const float BOOST_DRAIN = 30.0f;
static const float BOOST_RECHARGE = 8.0f;

static const float BOUNDS_X = 12.0f;
static const float BOUNDS_Y = 8.0f;

static const float CAM_POS_LERP = 0.07f;
static const float CAM_LOOK_LERP = 0.1f;

static const float FOG_START = 30.0f;
static const float FOG_END = 120.0f;

static const int SCORE_PER_RING = 50;

// ============================================================
// ENUMS
// ============================================================
enum GameState { MENU, PLAYING, GAME_OVER };

// ============================================================
// STRUCTURES
// ============================================================
struct Star {
    float x, y, z;
    float brightness;
    float twinkleSpeed;
    float twinklePhase;
};

struct Asteroid {
    float x, y, z;
    float radius;
    float rotX, rotY;
    float rotSpeedX, rotSpeedY;
    float r, g, b;
    bool active;
    float bumpAngles[4];
    float bumpDist[4];
};

struct Ring {
    float x, y, z;
    float r, g, b;
    bool active;
    float pulsePhase;
};

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float life;
    float maxLife;
    float r, g, b;
    float size;
    bool active;
};

// ============================================================
// GLOBALS
// ============================================================
static int gameState = MENU;
static int windowW = WINDOW_W;
static int windowH = WINDOW_H;

// Time
static float currentTime = 0.0f;
static float deltaTime = 0.016f;
static int lastTicks = 0;

// Ship
static float shipX = 0.0f;
static float shipY = 0.0f;
static float shipZ = 0.0f;
static float shipVelX = 0.0f;
static float shipVelY = 0.0f;
static float shipRoll = 0.0f;
static float shipPitch = 0.0f;
static float shipSpeed = SHIP_SPEED_START;
static float boostFuel = BOOST_FUEL_MAX;
static bool boosting = false;

// Camera
static float camPosX = 0.0f, camPosY = 3.5f, camPosZ = 5.5f;
static float camLookX = 0.0f, camLookY = 0.0f, camLookZ = -10.0f;
static float shakeAmount = 0.0f;

// Score
static int score = 0;
static int ringsCollected = 0;
static int highScore = 0;

// Stars
static Star stars[MAX_STARS];

// Asteroids
static Asteroid asteroids[MAX_ASTEROIDS];

// Rings
static Ring rings[MAX_RINGS];

// Particles
static Particle particles[MAX_PARTICLES];

// Input
static bool keyW = false, keyS = false, keyA = false, keyD = false;
static bool keyLeft = false, keyRight = false, keyUp = false, keyDown = false;
static bool keySpace = false, keyEnter = false;

// Menu pulse
static float menuPulse = 0.0f;

// Game over flash
static float deathFlash = 0.0f;
static bool prevPlaying = false;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
void initGame();
void initStars();
void spawnAsteroid(int i);
void spawnRing(int i);
void spawnParticle(float x, float y, float z, float vx, float vy, float vz,
                   float r, float g, float b, float life, float size);
void emitEngineTrail();
void emitRingBurst(float x, float y, float z, float r, float g, float b);
void emitExplosion(float x, float y, float z);
void updateGame(float dt);
void drawShip();
void drawAsteroid(const Asteroid& a);
void drawRing(const Ring& r);
void drawParticles();
void drawStars();
void drawHUD();
void drawMenu();
void drawGameOver();
void drawScene();
void display();
void reshape(int w, int h);
void timer(int v);
void keyboardDown(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void specialDown(int key, int x, int y);
void specialUp(int key, int x, int y);
void setupLighting();
void setupFog();
void setMaterial(float r, float g, float b, float ar, float ag, float ab,
                 float dr, float dg, float db, float sr, float sg, float sb,
                 float shine);
void setEmissive(float r, float g, float b);
void resetMaterial();
float lerp(float a, float b, float t);
float clampf(float v, float lo, float hi);

// ============================================================
// UTILITY
// ============================================================
float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ============================================================
// MATERIAL HELPERS
// ============================================================
void resetMaterial() {
    float defAmb[]  = {0.2f, 0.2f, 0.2f, 1.0f};
    float defDif[]  = {0.8f, 0.8f, 0.8f, 1.0f};
    float defSpec[] = {0.0f, 0.0f, 0.0f, 1.0f};
    float defEms[]  = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defDif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, defEms);
    glColor3f(0.8f, 0.8f, 0.8f);
}

void setMaterial(float ar, float ag, float ab,
                 float dr, float dg, float db,
                 float sr, float sg, float sb,
                 float er, float eg, float eb,
                 float shine) {
    float amb[]  = {ar, ag, ab, 1.0f};
    float dif[]  = {dr, dg, db, 1.0f};
    float spec[] = {sr, sg, sb, 1.0f};
    float ems[]  = {er, eg, eb, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ems);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shine);
    glColor3f(dr, dg, db);
}

void setEmissive(float r, float g, float b) {
    float ems[] = {r, g, b, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ems);
}

// ============================================================
// STAR FIELD
// ============================================================
void initStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = ((float)rand() / RAND_MAX - 0.5f) * 200.0f;
        stars[i].y = ((float)rand() / RAND_MAX - 0.5f) * 150.0f;
        stars[i].z = -((float)rand() / RAND_MAX) * 200.0f - 10.0f;
        stars[i].brightness = 0.3f + (float)rand() / RAND_MAX * 0.7f;
        stars[i].twinkleSpeed = 1.0f + (float)rand() / RAND_MAX * 4.0f;
        stars[i].twinklePhase = (float)rand() / RAND_MAX * PI * 2.0f;
    }
}

void drawStars() {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_POINTS);
    for (int i = 0; i < MAX_STARS; i++) {
        float b = stars[i].brightness *
                  (0.5f + 0.5f * sinf(currentTime * stars[i].twinkleSpeed + stars[i].twinklePhase));
        float wb = b * 0.5f + 0.1f;
        glColor4f(wb, wb, b, 1.0f);
        glVertex3f(stars[i].x, stars[i].y, stars[i].z);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);
}

// ============================================================
// SPACESHIP DRAWING
// ============================================================
void drawShip() {
    glPushMatrix();
    glTranslatef(shipX, shipY, shipZ);
    glRotatef(shipRoll * 20.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(shipPitch * 15.0f, 1.0f, 0.0f, 0.0f);

    // Model is built facing -Z already after 180 Y rotation
    // The whole ship model is rotated 180 around Y so nose points -Z
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    // --- Main body (dark sphere) ---
    setMaterial(0.05f, 0.05f, 0.08f,
                0.15f, 0.15f, 0.25f,
                0.3f, 0.3f, 0.4f,
                0.0f, 0.0f, 0.0f,
                30.0f);
    glPushMatrix();
    glScalef(0.6f, 0.5f, 1.2f);
    glutSolidSphere(1.0, 16, 12);
    glPopMatrix();

    // --- Nose cone (cylinder at +Z, which is the front after rotation) ---
    setMaterial(0.06f, 0.06f, 0.1f,
                0.2f, 0.2f, 0.3f,
                0.4f, 0.4f, 0.5f,
                0.0f, 0.0f, 0.0f,
                40.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, 0.15, 0.35, 0.8, 12, 1);
    gluDeleteQuadric(q);
    glPopMatrix();

    // --- Cockpit canopy (glowing cyan emissive sphere) ---
    setMaterial(0.0f, 0.3f, 0.3f,
                0.0f, 0.6f, 0.6f,
                1.0f, 1.0f, 1.0f,
                0.0f, 0.8f, 0.8f,
                80.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.2f, 0.5f);
    glScalef(1.0f, 0.6f, 1.0f);
    glutSolidSphere(0.25, 12, 8);
    glPopMatrix();

    // --- Wings (dark flat boxes extending to +/- X) ---
    setMaterial(0.04f, 0.04f, 0.07f,
                0.12f, 0.12f, 0.2f,
                0.2f, 0.2f, 0.3f,
                0.0f, 0.0f, 0.0f,
                20.0f);

    // Left wing
    glPushMatrix();
    glTranslatef(-0.8f, 0.0f, -0.1f);
    glScalef(1.2f, 0.08f, 0.7f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right wing
    glPushMatrix();
    glTranslatef(0.8f, 0.0f, -0.1f);
    glScalef(1.2f, 0.08f, 0.7f);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- Wing tips (glowing blue emissive spheres) ---
    setMaterial(0.0f, 0.1f, 0.4f,
                0.0f, 0.2f, 0.8f,
                1.0f, 1.0f, 1.0f,
                0.1f, 0.4f, 1.0f,
                80.0f);

    // Left wing tip
    glPushMatrix();
    glTranslatef(-1.4f, 0.0f, -0.1f);
    glutSolidSphere(0.1, 8, 6);
    glPopMatrix();

    // Right wing tip
    glPushMatrix();
    glTranslatef(1.4f, 0.0f, -0.1f);
    glutSolidSphere(0.1, 8, 6);
    glPopMatrix();

    // --- Engine nacelles (dark boxes at -Z) ---
    setMaterial(0.05f, 0.05f, 0.08f,
                0.18f, 0.18f, 0.25f,
                0.3f, 0.3f, 0.3f,
                0.0f, 0.0f, 0.0f,
                20.0f);

    // Left nacelle
    glPushMatrix();
    glTranslatef(-0.5f, -0.1f, -0.8f);
    glScalef(0.25f, 0.2f, 0.5f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right nacelle
    glPushMatrix();
    glTranslatef(0.5f, -0.1f, -0.8f);
    glScalef(0.25f, 0.2f, 0.5f);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- Engine exhaust (glowing blue emissive spheres, pulsing) ---
    float exhaustPulse = 0.7f + 0.3f * sinf(currentTime * 15.0f);
    float eb = boosting ? 1.0f : exhaustPulse;
    setMaterial(0.0f, 0.05f * eb, 0.2f * eb,
                0.0f, 0.2f * eb, 0.8f * eb,
                1.0f, 1.0f, 1.0f,
                0.2f * eb, 0.5f * eb, 1.0f * eb,
                90.0f);

    float exSize = boosting ? 0.15f : 0.1f;
    exSize *= (0.8f + 0.2f * sinf(currentTime * 20.0f));

    // Left exhaust
    glPushMatrix();
    glTranslatef(-0.5f, -0.1f, -1.1f);
    glutSolidSphere(exSize, 8, 6);
    glPopMatrix();

    // Right exhaust
    glPushMatrix();
    glTranslatef(0.5f, -0.1f, -1.1f);
    glutSolidSphere(exSize, 8, 6);
    glPopMatrix();

    // --- Tail fin (dark box on top) ---
    setMaterial(0.05f, 0.05f, 0.08f,
                0.15f, 0.15f, 0.22f,
                0.2f, 0.2f, 0.3f,
                0.0f, 0.0f, 0.0f,
                20.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.35f, -0.3f);
    glScalef(0.06f, 0.35f, 0.4f);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- Neon stripe (glowing cyan emissive box along body center) ---
    float stripePulse = 0.7f + 0.3f * sinf(currentTime * 3.0f);
    setMaterial(0.0f, 0.15f * stripePulse, 0.15f * stripePulse,
                0.0f, 0.4f * stripePulse, 0.4f * stripePulse,
                1.0f, 1.0f, 1.0f,
                0.0f, 0.9f * stripePulse, 0.9f * stripePulse,
                90.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    glScalef(0.04f, 0.04f, 1.8f);
    glutSolidCube(1.0);
    glPopMatrix();

    glPopMatrix();
}

// ============================================================
// ASTEROID DRAWING
// ============================================================
void drawAsteroid(const Asteroid& a) {
    glPushMatrix();
    glTranslatef(a.x, a.y, a.z);
    glRotatef(a.rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(a.rotY, 0.0f, 1.0f, 0.0f);

    // Main body
    setMaterial(0.1f, 0.08f, 0.06f,
                a.r, a.g, a.b,
                0.1f, 0.1f, 0.1f,
                0.0f, 0.0f, 0.0f,
                5.0f);
    glutSolidSphere(a.radius, 10, 8);

    // Surface bumps
    for (int i = 0; i < 4; i++) {
        float angle = a.bumpAngles[i];
        float dist = a.bumpDist[i];
        float bx = cosf(angle) * dist * a.radius;
        float by = sinf(angle) * dist * a.radius;
        float bz = cosf(angle * 0.7f + 1.0f) * dist * a.radius * 0.5f;
        glPushMatrix();
        glTranslatef(bx, by, bz);
        glutSolidSphere(a.radius * 0.25f, 6, 5);
        glPopMatrix();
    }

    glPopMatrix();
}

// ============================================================
// RING DRAWING
// ============================================================
void drawRing(const Ring& ring) {
    glPushMatrix();
    glTranslatef(ring.x, ring.y, ring.z);

    float pulse = 0.6f + 0.4f * sinf(currentTime * 4.0f + ring.pulsePhase);
    float cr = ring.r * pulse;
    float cg = ring.g * pulse;
    float cb = ring.b * pulse;

    // Ring segments (cylinders arranged in a circle)
    float ringRadius = 1.5f;
    float segLen = 2.0f * PI * ringRadius / RING_SEGMENTS;
    float segRadius = 0.08f;

    for (int i = 0; i < RING_SEGMENTS; i++) {
        float angle = (float)i / RING_SEGMENTS * 2.0f * PI;
        float nx = cosf(angle) * ringRadius;
        float ny = sinf(angle) * ringRadius;

        glPushMatrix();
        glTranslatef(nx, ny, 0.0f);
        // Orient cylinder outward
        float degAngle = angle / PI * 180.0f;
        glRotatef(degAngle, 0.0f, 0.0f, 1.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        setMaterial(cr * 0.3f, cg * 0.3f, cb * 0.3f,
                    cr, cg, cb,
                    1.0f, 1.0f, 1.0f,
                    cr * 0.8f, cg * 0.8f, cb * 0.8f,
                    80.0f);

        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, segRadius, segRadius, segLen * 0.9f, 6, 1);
        gluDeleteQuadric(q);
        glPopMatrix();
    }

    // Inner glowing sphere
    setMaterial(cr * 0.5f, cg * 0.5f, cb * 0.5f,
                cr, cg, cb,
                1.0f, 1.0f, 1.0f,
                cr, cg, cb,
                90.0f);
    glutSolidSphere(0.3f * pulse, 12, 8);

    glPopMatrix();
}

// ============================================================
// PARTICLE SYSTEM
// ============================================================
void spawnParticle(float x, float y, float z, float vx, float vy, float vz,
                   float r, float g, float b, float life, float size) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].z = z;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].vz = vz;
            particles[i].life = life;
            particles[i].maxLife = life;
            particles[i].r = r;
            particles[i].g = g;
            particles[i].b = b;
            particles[i].size = size;
            particles[i].active = true;
            return;
        }
    }
}

void emitEngineTrail() {
    float baseX = shipX;
    float baseY = shipY - 0.1f;
    float baseZ = shipZ + 1.1f; // +Z is behind (after model rotation)

    for (int side = -1; side <= 1; side += 2) {
        float ox = baseX + side * 0.5f;
        float spreadX = ((float)rand() / RAND_MAX - 0.5f) * 0.15f;
        float spreadY = ((float)rand() / RAND_MAX - 0.5f) * 0.15f;
        float spreadZ = ((float)rand() / RAND_MAX) * 0.5f + 0.3f;

        float life = 0.4f + (float)rand() / RAND_MAX * 0.4f;
        float size = 0.05f + (float)rand() / RAND_MAX * 0.1f;

        float cr = 0.1f + (float)rand() / RAND_MAX * 0.2f;
        float cg = 0.4f + (float)rand() / RAND_MAX * 0.3f;
        float cb = 0.8f + (float)rand() / RAND_MAX * 0.2f;

        if (boosting) {
            cr = 0.2f;
            cg = 0.6f;
            cb = 1.0f;
            size *= 1.5f;
        }

        spawnParticle(ox + spreadX, baseY + spreadY, baseZ + spreadZ,
                      spreadX * 2.0f, spreadY * 2.0f, spreadZ * 3.0f,
                      cr, cg, cb, life, size);
    }
}

void emitRingBurst(float x, float y, float z, float r, float g, float b) {
    for (int i = 0; i < 25; i++) {
        float angle = (float)i / 25.0f * 2.0f * PI;
        float speed = 3.0f + (float)rand() / RAND_MAX * 4.0f;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed;
        float vz = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
        float life = 0.5f + (float)rand() / RAND_MAX * 0.5f;
        float size = 0.08f + (float)rand() / RAND_MAX * 0.1f;
        spawnParticle(x, y, z, vx, vy, vz, r, g, b, life, size);
    }
}

void emitExplosion(float x, float y, float z) {
    for (int i = 0; i < 50; i++) {
        float vx = ((float)rand() / RAND_MAX - 0.5f) * 12.0f;
        float vy = ((float)rand() / RAND_MAX - 0.5f) * 12.0f;
        float vz = ((float)rand() / RAND_MAX - 0.5f) * 12.0f;
        float life = 0.8f + (float)rand() / RAND_MAX * 1.2f;
        float size = 0.1f + (float)rand() / RAND_MAX * 0.2f;
        float r = 0.8f + (float)rand() / RAND_MAX * 0.2f;
        float g = (float)rand() / RAND_MAX * 0.5f;
        float b = 0.0f;
        spawnParticle(x, y, z, vx, vy, vz, r, g, b, life, size);
    }
}

void updateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].vx * dt;
            particles[i].y += particles[i].vy * dt;
            particles[i].z += particles[i].vz * dt;
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f) {
                particles[i].active = false;
            }
        }
    }
}

void drawParticles() {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBegin(GL_POINTS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            float t = particles[i].life / particles[i].maxLife;
            glColor4f(particles[i].r * t, particles[i].g * t, particles[i].b * t, t);
            glPointSize(particles[i].size * 40.0f * t);
            glVertex3f(particles[i].x, particles[i].y, particles[i].z);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);
}

// ============================================================
// SPAWNING
// ============================================================
void spawnAsteroid(int i) {
    asteroids[i].active = true;
    asteroids[i].x = ((float)rand() / RAND_MAX - 0.5f) * 24.0f;
    asteroids[i].y = ((float)rand() / RAND_MAX - 0.5f) * 14.0f;
    asteroids[i].z = shipZ - 80.0f - (float)rand() / RAND_MAX * 40.0f;
    asteroids[i].radius = 0.4f + (float)rand() / RAND_MAX * 0.6f;
    asteroids[i].rotX = (float)rand() / RAND_MAX * 360.0f;
    asteroids[i].rotY = (float)rand() / RAND_MAX * 360.0f;
    asteroids[i].rotSpeedX = 10.0f + (float)rand() / RAND_MAX * 30.0f;
    asteroids[i].rotSpeedY = 10.0f + (float)rand() / RAND_MAX * 30.0f;

    float colorChoice = (float)rand() / RAND_MAX;
    if (colorChoice < 0.4f) {
        asteroids[i].r = 0.5f + (float)rand() / RAND_MAX * 0.2f;
        asteroids[i].g = 0.45f + (float)rand() / RAND_MAX * 0.15f;
        asteroids[i].b = 0.4f + (float)rand() / RAND_MAX * 0.1f;
    } else if (colorChoice < 0.7f) {
        asteroids[i].r = 0.4f + (float)rand() / RAND_MAX * 0.2f;
        asteroids[i].g = 0.3f + (float)rand() / RAND_MAX * 0.15f;
        asteroids[i].b = 0.25f + (float)rand() / RAND_MAX * 0.1f;
    } else {
        asteroids[i].r = 0.55f + (float)rand() / RAND_MAX * 0.2f;
        asteroids[i].g = 0.25f + (float)rand() / RAND_MAX * 0.1f;
        asteroids[i].b = 0.2f + (float)rand() / RAND_MAX * 0.1f;
    }

    for (int j = 0; j < 4; j++) {
        asteroids[i].bumpAngles[j] = (float)rand() / RAND_MAX * PI * 2.0f;
        asteroids[i].bumpDist[j] = 0.6f + (float)rand() / RAND_MAX * 0.4f;
    }
}

void spawnRing(int i) {
    rings[i].active = true;
    rings[i].x = ((float)rand() / RAND_MAX - 0.5f) * 20.0f;
    rings[i].y = ((float)rand() / RAND_MAX - 0.5f) * 12.0f;
    rings[i].z = shipZ - 60.0f - (float)rand() / RAND_MAX * 50.0f;
    rings[i].pulsePhase = (float)rand() / RAND_MAX * PI * 2.0f;

    int colorIdx = rand() % 3;
    if (colorIdx == 0) { rings[i].r = 0.0f; rings[i].g = 1.0f; rings[i].b = 0.8f; }
    else if (colorIdx == 1) { rings[i].r = 1.0f; rings[i].g = 0.3f; rings[i].b = 0.8f; }
    else { rings[i].r = 1.0f; rings[i].g = 0.7f; rings[i].b = 0.0f; }
}

// ============================================================
// GAME INIT
// ============================================================
void initGame() {
    shipX = 0.0f;
    shipY = 0.0f;
    shipZ = 0.0f;
    shipVelX = 0.0f;
    shipVelY = 0.0f;
    shipRoll = 0.0f;
    shipPitch = 0.0f;
    shipSpeed = SHIP_SPEED_START;
    boostFuel = BOOST_FUEL_MAX;
    boosting = false;
    score = 0;
    ringsCollected = 0;
    deathFlash = 0.0f;
    shakeAmount = 0.0f;

    camPosX = 0.0f;
    camPosY = 3.5f;
    camPosZ = 5.5f;
    camLookX = 0.0f;
    camLookY = 0.0f;
    camLookZ = -10.0f;

    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    for (int i = 0; i < MAX_ASTEROIDS; i++) spawnAsteroid(i);
    for (int i = 0; i < MAX_RINGS; i++) spawnRing(i);
}

// ============================================================
// COLLISION CHECK
// ============================================================
bool checkCollision(float ax, float ay, float az, float ar,
                    float bx, float by, float bz, float br) {
    float dx = ax - bx;
    float dy = ay - by;
    float dz = az - bz;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    return dist < (ar + br);
}

// ============================================================
// UPDATE GAME
// ============================================================
void updateGame(float dt) {
    // Ship speed increases over time
    shipSpeed += SPEED_INCREASE * dt;
    if (shipSpeed > SHIP_SPEED_MAX) shipSpeed = SHIP_SPEED_MAX;

    float effectiveSpeed = shipSpeed;
    if (boosting && boostFuel > 0.0f) {
        effectiveSpeed += BOOST_ADD;
        boostFuel -= BOOST_DRAIN * dt;
        if (boostFuel < 0.0f) boostFuel = 0.0f;
    } else {
        boosting = false;
        boostFuel += BOOST_RECHARGE * dt;
        if (boostFuel > BOOST_FUEL_MAX) boostFuel = BOOST_FUEL_MAX;
    }

    // Steering input
    float inputX = 0.0f;
    float inputY = 0.0f;
    if (keyA || keyLeft)  inputX -= 1.0f;
    if (keyD || keyRight) inputX += 1.0f;
    if (keyW || keyUp)    inputY += 1.0f;
    if (keyS || keyDown)  inputY -= 1.0f;

    // Velocity blending
    shipVelX = lerp(shipVelX, inputX * STEER_SPEED, 8.0f * dt);
    shipVelY = lerp(shipVelY, inputY * STEER_SPEED, 8.0f * dt);

    shipX += shipVelX * dt;
    shipY += shipVelY * dt;

    // Bounds
    shipX = clampf(shipX, -BOUNDS_X, BOUNDS_X);
    shipY = clampf(shipY, -BOUNDS_Y, BOUNDS_Y);

    // Ship tilt
    shipRoll = lerp(shipRoll, -inputX, 5.0f * dt);
    shipPitch = lerp(shipPitch, inputY, 5.0f * dt);

    // Move asteroids toward player
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            asteroids[i].z += effectiveSpeed * dt;
            asteroids[i].rotX += asteroids[i].rotSpeedX * dt;
            asteroids[i].rotY += asteroids[i].rotSpeedY * dt;

            if (asteroids[i].z > shipZ + 15.0f) {
                spawnAsteroid(i);
            }

            // Collision check
            if (checkCollision(shipX, shipY, shipZ, 0.8f,
                               asteroids[i].x, asteroids[i].y, asteroids[i].z,
                               asteroids[i].radius)) {
                emitExplosion(shipX, shipY, shipZ);
                shakeAmount = 2.0f;
                deathFlash = 1.0f;
                if (score > highScore) highScore = score;
                gameState = GAME_OVER;
                return;
            }
        }
    }

    // Move rings toward player
    for (int i = 0; i < MAX_RINGS; i++) {
        if (rings[i].active) {
            rings[i].z += effectiveSpeed * dt;

            if (rings[i].z > shipZ + 15.0f) {
                spawnRing(i);
            }

            // Collection check
            if (checkCollision(shipX, shipY, shipZ, 1.0f,
                               rings[i].x, rings[i].y, rings[i].z, 1.5f)) {
                emitRingBurst(rings[i].x, rings[i].y, rings[i].z,
                              rings[i].r, rings[i].g, rings[i].b);
                score += SCORE_PER_RING;
                ringsCollected++;
                rings[i].active = false;
                spawnRing(i);
            }
        }
    }

    // Engine trail
    if ((int)(currentTime * 40.0f) % 2 == 0) {
        emitEngineTrail();
    }

    // Particles
    updateParticles(dt);

    // Camera
    float targetCamX = shipX * 0.3f;
    float targetCamY = 3.5f + shipSpeed * 0.02f;
    float targetCamZ = shipZ + 5.5f;
    float targetLookX = shipX;
    float targetLookY = shipY;
    float targetLookZ = shipZ - 10.0f;

    camPosX = lerp(camPosX, targetCamX, CAM_POS_LERP);
    camPosY = lerp(camPosY, targetCamY, CAM_POS_LERP);
    camPosZ = lerp(camPosZ, targetCamZ, CAM_POS_LERP);
    camLookX = lerp(camLookX, targetLookX, CAM_LOOK_LERP);
    camLookY = lerp(camLookY, targetLookY, CAM_LOOK_LERP);
    camLookZ = lerp(camLookZ, targetLookZ, CAM_LOOK_LERP);

    // Screen shake decay
    if (shakeAmount > 0.01f) {
        shakeAmount *= 0.9f;
    } else {
        shakeAmount = 0.0f;
    }

    // Death flash fade
    if (deathFlash > 0.0f && gameState == GAME_OVER) {
        deathFlash -= dt * 2.0f;
        if (deathFlash < 0.0f) deathFlash = 0.0f;
    }

    prevPlaying = (gameState == PLAYING);
}

// ============================================================
// LIGHTING SETUP
// ============================================================
void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);

    // Main directional light (from above-front)
    float l0Pos[] = {0.3f, 1.0f, 0.5f, 0.0f};
    float l0Amb[] = {0.05f, 0.05f, 0.08f, 1.0f};
    float l0Dif[] = {0.6f, 0.6f, 0.7f, 1.0f};
    float l0Spc[] = {0.3f, 0.3f, 0.4f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, l0Amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, l0Spc);

    // Fill light (from below-behind, dim)
    float l1Pos[] = {-0.2f, -0.5f, -0.8f, 0.0f};
    float l1Amb[] = {0.02f, 0.02f, 0.03f, 1.0f};
    float l1Dif[] = {0.15f, 0.1f, 0.2f, 1.0f};
    float l1Spc[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, l1Pos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, l1Amb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, l1Dif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, l1Spc);

    // Ship headlight (spotlight from ship forward)
    float l2Pos[] = {shipX, shipY, shipZ - 2.0f, 1.0f};
    float l2Dir[] = {0.0f, 0.0f, -1.0f};
    float l2Amb[] = {0.0f, 0.0f, 0.0f, 1.0f};
    float l2Dif[] = {0.4f, 0.4f, 0.5f, 1.0f};
    float l2Spc[] = {0.2f, 0.2f, 0.3f, 1.0f};
    glLightfv(GL_LIGHT2, GL_POSITION, l2Pos);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, l2Dir);
    glLightfv(GL_LIGHT2, GL_AMBIENT, l2Amb);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, l2Dif);
    glLightfv(GL_LIGHT2, GL_SPECULAR, l2Spc);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 30.0f);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 10.0f);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.05f);
}

// ============================================================
// FOG SETUP
// ============================================================
void setupFog() {
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, FOG_START);
    glFogf(GL_FOG_END, FOG_END);
    float fogColor[] = {0.02f, 0.02f, 0.05f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 1.0f);
}

// ============================================================
// HUD
// ============================================================
void drawHUD() {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowW, 0, windowH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Score top-left
    glColor3f(0.0f, 1.0f, 0.8f);
    char buf[128];
    snprintf(buf, sizeof(buf), "SCORE: %d", score);
    glRasterPos2f(20, windowH - 30);
    for (const char* c = buf; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Rings count
    snprintf(buf, sizeof(buf), "RINGS: %d", ringsCollected);
    glRasterPos2f(20, windowH - 55);
    glColor3f(1.0f, 0.7f, 0.0f);
    for (const char* c = buf; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Speed top-right
    snprintf(buf, sizeof(buf), "SPEED: %.0f", shipSpeed + (boosting ? BOOST_ADD : 0));
    int len = (int)strlen(buf);
    glRasterPos2f(windowW - 20 - len * 10, windowH - 30);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (const char* c = buf; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Boost bar bottom-center
    float barW = 200.0f;
    float barH = 16.0f;
    float barX = (windowW - barW) / 2.0f;
    float barY = 40.0f;
    float fuelPct = boostFuel / BOOST_FUEL_MAX;

    // Background bar
    glColor4f(0.2f, 0.2f, 0.2f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(barX, barY);
    glVertex2f(barX + barW, barY);
    glVertex2f(barX + barW, barY + barH);
    glVertex2f(barX, barY + barH);
    glEnd();

    // Fuel fill
    if (fuelPct > 0.25f) {
        glColor3f(0.0f, 0.4f, 1.0f);
    } else {
        glColor3f(1.0f, 0.2f, 0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(barX + 2, barY + 2);
    glVertex2f(barX + 2 + (barW - 4) * fuelPct, barY + 2);
    glVertex2f(barX + 2 + (barW - 4) * fuelPct, barY + barH - 2);
    glVertex2f(barX + 2, barY + barH - 2);
    glEnd();

    // Label
    glColor3f(0.3f, 0.5f, 1.0f);
    const char* label = "BOOST [SPACE]";
    int lLen = (int)strlen(label);
    glRasterPos2f((windowW - lLen * 9) / 2.0f, barY + barH + 8);
    for (const char* c = label; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);
}

// ============================================================
// MENU SCREEN
// ============================================================
void drawMenu() {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowW, 0, windowH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Black overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(windowW, 0);
    glVertex2f(windowW, windowH);
    glVertex2f(0, windowH);
    glEnd();
    glDisable(GL_BLEND);

    menuPulse += deltaTime;
    float pulse = 0.7f + 0.3f * sinf(menuPulse * 3.0f);

    // Title: "COSMIC DRIFT"
    glColor3f(0.0f, pulse, pulse * 0.9f);
    const char* title = "COSMIC DRIFT";
    int titleLen = (int)strlen(title);
    glRasterPos2f((windowW - titleLen * 18) / 2, windowH * 0.65f);
    for (const char* c = title; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }

    // Subtitle
    glColor3f(1.0f, 0.3f, 0.8f);
    const char* sub = "Space Runner";
    int subLen = (int)strlen(sub);
    glRasterPos2f((windowW - subLen * 11) / 2, windowH * 0.65f - 35);
    for (const char* c = sub; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Controls list
    glColor3f(0.7f, 0.7f, 0.7f);
    const char* controls[] = {
        "WASD / Arrow Keys - Steer",
        "SPACE - Boost",
        "ESC - Menu",
        "",
        "Collect rings for points!",
        "Avoid asteroids!"
    };
    float startY = windowH * 0.42f;
    for (int i = 0; i < 6; i++) {
        int cLen = (int)strlen(controls[i]);
        glRasterPos2f((windowW - cLen * 9) / 2, startY - i * 22);
        for (const char* c = controls[i]; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }

    // High score
    if (highScore > 0) {
        glColor3f(1.0f, 0.8f, 0.0f);
        char hbuf[64];
        snprintf(hbuf, sizeof(hbuf), "HIGH SCORE: %d", highScore);
        int hLen = (int)strlen(hbuf);
        glRasterPos2f((windowW - hLen * 9) / 2, startY - 6 * 22 - 10);
        for (const char* c = hbuf; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }

    // Press ENTER
    float enterPulse = 0.5f + 0.5f * sinf(menuPulse * 4.0f);
    glColor3f(0.0f, enterPulse, enterPulse);
    const char* prompt = "Press ENTER to Launch";
    int pLen = (int)strlen(prompt);
    glRasterPos2f((windowW - pLen * 10) / 2, windowH * 0.15f);
    for (const char* c = prompt; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);
}

// ============================================================
// GAME OVER SCREEN
// ============================================================
void drawGameOver() {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowW, 0, windowH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Red flash overlay (fading)
    if (deathFlash > 0.0f) {
        glColor4f(1.0f, 0.0f, 0.0f, deathFlash * 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(windowW, 0);
        glVertex2f(windowW, windowH);
        glVertex2f(0, windowH);
        glEnd();
    }

    // Dark overlay
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(windowW, 0);
    glVertex2f(windowW, windowH);
    glVertex2f(0, windowH);
    glEnd();

    glDisable(GL_BLEND);

    // "DESTROYED" title
    glColor3f(1.0f, 0.1f, 0.0f);
    const char* title = "DESTROYED";
    int tLen = (int)strlen(title);
    glRasterPos2f((windowW - tLen * 18) / 2, windowH * 0.6f);
    for (const char* c = title; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }

    // Score
    glColor3f(0.0f, 1.0f, 0.8f);
    char buf[128];
    snprintf(buf, sizeof(buf), "Score: %d", score);
    int sLen = (int)strlen(buf);
    glRasterPos2f((windowW - sLen * 10) / 2, windowH * 0.48f);
    for (const char* c = buf; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Rings
    snprintf(buf, sizeof(buf), "Rings: %d", ringsCollected);
    int rLen = (int)strlen(buf);
    glRasterPos2f((windowW - rLen * 10) / 2, windowH * 0.42f);
    for (const char* c = buf; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // High score
    if (score >= highScore && score > 0) {
        float hp = 0.5f + 0.5f * sinf(currentTime * 4.0f);
        glColor3f(1.0f, 0.8f * hp, 0.0f);
        const char* hs = "NEW HIGH SCORE!";
        int hsLen = (int)strlen(hs);
        glRasterPos2f((windowW - hsLen * 10) / 2, windowH * 0.35f);
        for (const char* c = hs; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }

    // Options
    glColor3f(0.7f, 0.7f, 0.7f);
    const char* opt = "SPACE = Retry  |  ENTER = Menu";
    int oLen = (int)strlen(opt);
    glRasterPos2f((windowW - oLen * 9) / 2, windowH * 0.2f);
    for (const char* c = opt; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);
}

// ============================================================
// SCENE DRAW
// ============================================================
void drawScene() {
    // Draw background stars (no fog/lighting)
    drawStars();

    // Ship
    drawShip();

    // Asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            drawAsteroid(asteroids[i]);
        }
    }

    // Rings
    for (int i = 0; i < MAX_RINGS; i++) {
        if (rings[i].active) {
            drawRing(rings[i]);
        }
    }

    // Particles
    drawParticles();
}

// ============================================================
// DISPLAY
// ============================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == PLAYING || gameState == GAME_OVER) {
        // Camera
        float shakeX = 0.0f, shakeY = 0.0f;
        if (shakeAmount > 0.01f) {
            shakeX = ((float)rand() / RAND_MAX - 0.5f) * shakeAmount;
            shakeY = ((float)rand() / RAND_MAX - 0.5f) * shakeAmount;
        }

        gluLookAt(camPosX + shakeX, camPosY + shakeY, camPosZ,
                  camLookX, camLookY, camLookZ,
                  0.0f, 1.0f, 0.0f);

        setupLighting();
        setupFog();

        drawScene();
    }

    if (gameState == MENU) {
        // Draw a subtle rotating camera for the menu background
        float menuCamDist = 12.0f;
        float menuAngle = currentTime * 0.3f;
        gluLookAt(cosf(menuAngle) * menuCamDist, 4.0f, sinf(menuAngle) * menuCamDist,
                  0.0f, 0.0f, -10.0f,
                  0.0f, 1.0f, 0.0f);

        setupLighting();
        setupFog();

        drawStars();

        // Draw some asteroids in background
        for (int i = 0; i < 8; i++) {
            Asteroid tempA;
            tempA.x = sinf((float)i * 1.3f) * 8.0f;
            tempA.y = cosf((float)i * 0.7f) * 4.0f;
            tempA.z = -20.0f - (float)i * 5.0f;
            tempA.radius = 0.5f + (float)i * 0.1f;
            tempA.rotX = currentTime * 10.0f + (float)i * 40.0f;
            tempA.rotY = currentTime * 15.0f + (float)i * 30.0f;
            tempA.r = 0.5f;
            tempA.g = 0.4f;
            tempA.b = 0.35f;
            for (int j = 0; j < 4; j++) {
                tempA.bumpAngles[j] = (float)j * 1.5f;
                tempA.bumpDist[j] = 0.8f;
            }
            drawAsteroid(tempA);
        }

        drawMenu();
    }

    if (gameState == GAME_OVER) {
        drawGameOver();
    }

    glutSwapBuffers();
}

// ============================================================
// RESHAPE
// ============================================================
void reshape(int w, int h) {
    if (h == 0) h = 1;
    windowW = w;
    windowH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 0.1, 300.0);
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// TIMER
// ============================================================
void timer(int v) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (now - lastTicks) / 1000.0f;
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    if (deltaTime < 0.001f) deltaTime = 0.001f;
    lastTicks = now;
    currentTime += deltaTime;

    if (gameState == PLAYING) {
        updateGame(deltaTime);
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ============================================================
// INPUT
// ============================================================
void keyboardDown(unsigned char key, int x, int y) {
    switch (key) {
        case 'w': case 'W': keyW = true; break;
        case 's': case 'S': keyS = true; break;
        case 'a': case 'A': keyA = true; break;
        case 'd': case 'D': keyD = true; break;
        case ' ':
            keySpace = true;
            if (gameState == PLAYING) {
                boosting = true;
            }
            if (gameState == GAME_OVER) {
                gameState = PLAYING;
                initGame();
            }
            break;
        case 13: // ENTER
            keyEnter = true;
            if (gameState == MENU) {
                gameState = PLAYING;
                initGame();
            } else if (gameState == GAME_OVER) {
                gameState = MENU;
            }
            break;
        case 27: // ESC
            if (gameState == PLAYING || gameState == GAME_OVER) {
                gameState = MENU;
            }
            break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    switch (key) {
        case 'w': case 'W': keyW = false; break;
        case 's': case 'S': keyS = false; break;
        case 'a': case 'A': keyA = false; break;
        case 'd': case 'D': keyD = false; break;
        case ' ':
            keySpace = false;
            boosting = false;
            break;
        case 13: keyEnter = false; break;
    }
}

void specialDown(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:  keyLeft = true; break;
        case GLUT_KEY_RIGHT: keyRight = true; break;
        case GLUT_KEY_UP:    keyUp = true; break;
        case GLUT_KEY_DOWN:  keyDown = true; break;
    }
}

void specialUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:  keyLeft = false; break;
        case GLUT_KEY_RIGHT: keyRight = false; break;
        case GLUT_KEY_UP:    keyUp = false; break;
        case GLUT_KEY_DOWN:  keyDown = false; break;
    }
}

// ============================================================
// MAIN
// ============================================================
int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_W, WINDOW_H);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Cosmic Drift - Space Runner");

    // GL state
    glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)WINDOW_W / (double)WINDOW_H, 0.1, 300.0);
    glMatrixMode(GL_MODELVIEW);

    // Init stars
    initStars();

    // Callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);

    lastTicks = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
