#include <windows.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>

// ================= CONSTANTS =================
#define PI            3.14159f
#define WINDOW_W      1200
#define WINDOW_H      700
#define MAX_SMOKE     50
#define MAX_RAIN      220
#define MAX_STARS     130
#define MAX_LEAVES    80
#define MAX_LAMPS     4

// ================= DAY-NIGHT CYCLE =================
float timeOfDay  = 0.42f;
float daySpeed   = 0.00018f;
bool  pauseTime  = false;

// ================= PLAYER BIRD =================
struct PlayerBird {
    float x, y, vx, vy, wingAngle;
    int   wingDir, score;
};
PlayerBird bird = { 200.0f, 560.0f, 0.0f, 0.0f, 20.0f, 1, 0 };

bool keyDown[4] = { false, false, false, false };

// ================= TRAIN =================
bool  trainMoving     = false;
float trainX          = -400.0f;
float trainSpeed      = 0.0f;
const float TRAIN_MAX = 3.5f;

// ================= SMOKE =================
struct SmokeParticle { float x,y,vx,vy,life,size; bool active; };
SmokeParticle smoke[MAX_SMOKE];

// ================= RAIN =================
struct RainDrop { float x,y,speed,length; };
RainDrop rain[MAX_RAIN];
bool rainActive = false;

// ================= LIGHTNING =================
float lightningFlash    = 0.0f;
float lightningTimer    = 0.0f;
float lightningInterval = 6.0f;

// ================= STARS =================
struct Star { float x,y,brightness,twinkleSpeed,twinklePhase; };
Star stars[MAX_STARS];

// ================= CLOUDS =================
struct Cloud { float x,y,speed,scale; };
Cloud clouds[3] = {
    { 180.0f, 620.0f, 0.40f, 1.00f },
    { 550.0f, 580.0f, 0.60f, 0.75f },
    { 900.0f, 645.0f, 0.30f, 0.85f }
};

// ================= AUTO BIRDS =================
struct AutoBird { float x,y,speed,scale,wingAngle; int wingDir; };
AutoBird autoBirds[3] = {
    { 400.0f, 530.0f, 1.2f, 0.80f,  0.0f, 1 },
    { 700.0f, 580.0f, 0.9f, 0.60f, 10.0f, 1 },
    { 950.0f, 555.0f, 1.5f, 0.70f,  5.0f, 1 }
};

// ================= TOWER BIRD =================
struct TowerBird {
    float x, y;
    float vx, vy;
    float wingAngle;
    int   wingDir;
    int   state;
    float seatTimer;
};
TowerBird tBird = { -60.0f, 460.0f, 1.4f, 0.0f, 0.0f, 1, 0, 0.0f };
const float TOWER_SEAT_X = 304.0f;
const float TOWER_SEAT_Y = 548.0f;

// ================= FOOTBALL =================
float ballX    = 520.0f;
float ballVX   =   1.8f;
float ballSpin =   0.0f;

// ================= GOAL CELEBRATION =================
bool  goalCelebration = false;
float goalTimer       = 0.0f;

// ================= CHILDREN =================
struct Child {
    float x;
    float targetX;
    float speed;
    float sr,sg,sb;
    float pr,pg,pb;
    int   basepose;
    int   animFrame;
    float animTimer;
};
Child children[5] = {
    { 320.0f, 500.0f, 0.8f, 0.90f,0.10f,0.10f, 0.10f,0.20f,0.80f, 1, 0, 0.0f },
    { 190.0f, 340.0f, 0.6f, 0.15f,0.35f,0.90f, 0.15f,0.15f,0.35f, 0, 0, 0.0f },
    { 520.0f, 400.0f, 0.5f, 0.10f,0.75f,0.25f, 0.40f,0.40f,0.40f, 2, 0, 0.0f },
    { 750.0f, 600.0f, 0.7f, 0.95f,0.50f,0.05f, 0.15f,0.12f,0.30f, 3, 0, 0.0f },
    { 980.0f, 980.0f, 0.0f, 0.95f,0.90f,0.10f, 0.10f,0.20f,0.75f, 4, 0, 0.0f }
};

// ================= LEAVES =================
struct Leaf {
    float x,y,vx,vy,angle,angleV,life,size;
    bool  active;
};
Leaf leaves[MAX_LEAVES];

// ================= WATER SHIMMER (kept for fog use) =================
float waterShimmer = 0.0f;

// ================= FOG =================
float fogAlpha = 0.0f;

// ================= STREET LAMP FLICKER =================
float lampFlicker[MAX_LAMPS];

// ================= WINDOW FLICKER =================
float winFlicker[4];

// ================= FPS =================
int   frameCount  = 0;
float fps         = 0.0f;
int   lastFPSTime = 0;

// =============================================================
//  UTILITIES
// =============================================================
float lerp(float a, float b, float t) { return a + (b-a)*t; }
float clampf(float v, float lo, float hi) { return v<lo?lo:v>hi?hi:v; }

float getNightFactor() {
    if (timeOfDay >= 0.85f || timeOfDay < 0.15f) return 1.0f;
    if (timeOfDay >= 0.75f && timeOfDay < 0.85f) return (timeOfDay-0.75f)/0.10f;
    if (timeOfDay >= 0.15f && timeOfDay < 0.25f) return 1.0f-(timeOfDay-0.15f)/0.10f;
    return 0.0f;
}

float getDayFactor() {
    if (timeOfDay >= 0.35f && timeOfDay <= 0.65f) return 1.0f;
    if (timeOfDay >= 0.25f && timeOfDay < 0.35f) return (timeOfDay-0.25f)/0.10f;
    if (timeOfDay >  0.65f && timeOfDay <= 0.75f) return 1.0f-(timeOfDay-0.65f)/0.10f;
    return 0.0f;
}

float getDuskFactor() {
    float d = 0.0f;
    if (timeOfDay >= 0.25f && timeOfDay < 0.35f) d = 1.0f-(timeOfDay-0.25f)/0.10f;
    if (timeOfDay >= 0.65f && timeOfDay < 0.75f) d = (timeOfDay-0.65f)/0.10f;
    if (timeOfDay >= 0.75f && timeOfDay < 0.85f) d = 1.0f-(timeOfDay-0.75f)/0.10f;
    return clampf(d, 0.0f, 1.0f);
}

void getSkyColor(float t, float& r, float& g, float& b, bool top) {
    if (t < 0.25f) {
        float f = t/0.25f;
        if (top){ r=0.02f+f*0.55f; g=0.02f+f*0.35f; b=0.15f+f*0.55f; }
        else    { r=0.05f+f*0.75f; g=0.05f+f*0.45f; b=0.20f+f*0.55f; }
    } else if (t < 0.35f) {
        float f=(t-0.25f)/0.10f;
        if (top){ r=lerp(0.57f,0.40f,f); g=lerp(0.37f,0.70f,f); b=lerp(0.70f,0.95f,f); }
        else    { r=lerp(0.80f,0.65f,f); g=lerp(0.50f,0.88f,f); b=lerp(0.75f,1.00f,f); }
    } else if (t < 0.65f) {
        if (top){ r=0.40f; g=0.70f; b=0.95f; }
        else    { r=0.65f; g=0.88f; b=1.00f; }
    } else if (t < 0.75f) {
        float f=(t-0.65f)/0.10f;
        if (top){ r=lerp(0.40f,0.75f,f); g=lerp(0.70f,0.35f,f); b=lerp(0.95f,0.30f,f); }
        else    { r=lerp(0.65f,0.90f,f); g=lerp(0.88f,0.50f,f); b=lerp(1.00f,0.30f,f); }
    } else if (t < 0.85f) {
        float f=(t-0.75f)/0.10f;
        if (top){ r=lerp(0.75f,0.02f,f); g=lerp(0.35f,0.02f,f); b=lerp(0.30f,0.15f,f); }
        else    { r=lerp(0.90f,0.05f,f); g=lerp(0.50f,0.05f,f); b=lerp(0.30f,0.20f,f); }
    } else {
        if (top){ r=0.02f; g=0.02f; b=0.15f; }
        else    { r=0.05f; g=0.05f; b=0.20f; }
    }
}

// =============================================================
//  INITIALIZERS
// =============================================================
void initSmoke() { for(int i=0;i<MAX_SMOKE;i++) smoke[i].active=false; }

void initRain() {
    for(int i=0;i<MAX_RAIN;i++){
        rain[i].x      = (float)(rand()%1200);
        rain[i].y      = (float)(rand()%700+100);
        rain[i].speed  = 6.0f+(rand()%40)*0.1f;
        rain[i].length = 10.0f+(rand()%8);
    }
}

void initStars() {
    for(int i=0;i<MAX_STARS;i++){
        stars[i].x            = (float)(rand()%1200);
        stars[i].y            = 360.0f+(float)(rand()%340);
        stars[i].brightness   = 0.5f+(rand()%50)*0.01f;
        stars[i].twinkleSpeed = 0.02f+(rand()%30)*0.001f;
        stars[i].twinklePhase = (float)(rand()%628)*0.01f;
    }
}

void initLeaves() { for(int i=0;i<MAX_LEAVES;i++) leaves[i].active=false; }

void initFlicker() {
    for(int i=0;i<MAX_LAMPS;i++) lampFlicker[i]=(float)(rand()%100)*0.01f;
    for(int i=0;i<4;i++)         winFlicker[i] =(float)(rand()%100)*0.01f;
}

// =============================================================
//  PRIMITIVE HELPERS
// =============================================================
void drawCircle(float cx,float cy,float rx,float ry){
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for(int i=0;i<=360;i++){
        float a=i*PI/180.0f;
        glVertex2f(cx+rx*cosf(a),cy+ry*sinf(a));
    }
    glEnd();
}

void drawRect(float x,float y,float w,float h){
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}

void drawTriangle(float x1,float y1,float x2,float y2,float x3,float y3){
    glBegin(GL_TRIANGLES);
    glVertex2f(x1,y1); glVertex2f(x2,y2); glVertex2f(x3,y3);
    glEnd();
}

// =============================================================
//  SKY
// =============================================================
void drawSky(){
    float tr,tg,tb,br,bg,bb;
    getSkyColor(timeOfDay,tr,tg,tb,true);
    getSkyColor(timeOfDay,br,bg,bb,false);
    glBegin(GL_QUADS);
    glColor3f(tr,tg,tb); glVertex2f(0,700); glVertex2f(1200,700);
    glColor3f(br,bg,bb); glVertex2f(1200,280); glVertex2f(0,280);
    glEnd();
}

// =============================================================
//  STARS
// =============================================================
void drawStars(){
    float nf=getNightFactor();
    if(nf<=0.0f) return;
    for(int i=0;i<MAX_STARS;i++){
        float tw=stars[i].brightness+0.3f*sinf(stars[i].twinklePhase);
        tw=clampf(tw*nf,0.0f,1.0f);
        glColor3f(tw,tw,tw*0.9f+0.1f);
        drawCircle(stars[i].x,stars[i].y,1.5f,1.5f);
    }
}

// =============================================================
//  MOON
// =============================================================
void drawMoon(){
    float nf=getNightFactor();
    if(nf<=0.0f) return;
    float mx=155.0f, my=620.0f, r=38.0f;
    glColor3f(0.55f*nf,0.55f*nf,0.40f*nf); drawCircle(mx,my,r+28.0f,r+28.0f);
    glColor3f(0.70f*nf,0.70f*nf,0.52f*nf); drawCircle(mx,my,r+14.0f,r+14.0f);
    glColor3f(0.96f*nf,0.96f*nf,0.82f*nf); drawCircle(mx,my,r,r);
    float sr,sg,sb; getSkyColor(timeOfDay,sr,sg,sb,true);
    glColor3f(sr,sg,sb); drawCircle(mx+15.0f,my-5.0f,r-4.0f,r-4.0f);
    glColor3f(0.78f*nf,0.76f*nf,0.60f*nf);
    drawCircle(mx-10.0f,my+8.0f,6.0f,6.0f);
    drawCircle(mx+5.0f,my-12.0f,4.0f,4.0f);
    drawCircle(mx-4.0f,my-3.0f,3.0f,3.0f);
}

// =============================================================
//  RIGHT SUN
// =============================================================
void drawSun(){
    float df=getDayFactor();
    if(df<=0.0f) return;
    glColor3f(1.0f,0.97f*df,0.60f); drawCircle(960,335,60,60);
    glColor3f(1.0f,0.92f*df,0.10f); drawCircle(960,335,48,48);
}

// =============================================================
//  LEFT SUN (detailed)
// =============================================================
void drawLeftSun(){
    float df=getDayFactor();
    if(df<=0.0f) return;
    float cx=105.0f,cy=510.0f,r=68.0f;
    glColor3f(1.0f,0.97f,0.70f*df); drawCircle(cx,cy,r+55.0f,r+55.0f);
    glColor3f(1.0f,0.94f,0.45f*df); drawCircle(cx,cy,r+32.0f,r+32.0f);
    glColor3f(1.0f,0.90f,0.25f*df); drawCircle(cx,cy,r+16.0f,r+16.0f);
    for(int i=0;i<16;i++){
        float aMid=(float)i*(2.0f*PI/16.0f);
        float aHalf=(PI/16.0f)*0.45f;
        float rayLen=(i%2==0)?r+52.0f:r+28.0f;
        float baseR=r*0.92f;
        float tipX=cx+rayLen*cosf(aMid),tipY=cy+rayLen*sinf(aMid);
        float b1x=cx+baseR*cosf(aMid-aHalf),b1y=cy+baseR*sinf(aMid-aHalf);
        float b2x=cx+baseR*cosf(aMid+aHalf),b2y=cy+baseR*sinf(aMid+aHalf);
        glBegin(GL_TRIANGLES);
          glColor3f(1.0f,0.82f,0.10f*df); glVertex2f(b1x,b1y); glVertex2f(b2x,b2y);
          glColor3f(1.0f,0.98f,0.82f*df); glVertex2f(tipX,tipY);
        glEnd();
    }
    glBegin(GL_TRIANGLE_FAN);
      glColor3f(1.0f,0.99f,0.88f*df); glVertex2f(cx,cy);
      for(int i=0;i<=360;i++){
          float a=i*PI/180.0f;
          glColor3f(1.0f,0.75f,0.05f*df);
          glVertex2f(cx+r*cosf(a),cy+r*sinf(a));
      }
    glEnd();
    glColor3f(1.0f,1.0f,0.95f*df);
    drawCircle(cx-14.0f,cy+16.0f,r*0.30f,r*0.30f);
}

// =============================================================
//  GROUND
// =============================================================
void drawGround(){
    float nf=getNightFactor();
    float dim=1.0f-nf*0.60f;
    glColor3f(0.87f*dim,0.78f*dim,0.55f*dim);
    drawRect(0,150,1200,135);
}

// =============================================================
//  BOTTOM GRASS
// =============================================================
void drawBottomGrass(){
    float nf=getNightFactor();
    float dim=1.0f-nf*0.60f;
    glColor3f(0.15f*dim,0.72f*dim,0.15f*dim);
    drawRect(0,0,1200,158);
    glColor3f(0.05f*dim,0.55f*dim,0.05f*dim);
    for(int x=-28;x<1200;x+=28)
        drawTriangle((float)x,158.0f,(float)x+14,200.0f,(float)x+28,158.0f);
    glColor3f(0.25f*dim,0.85f*dim,0.25f*dim);
    for(int x=-42;x<1200;x+=28)
        drawTriangle((float)x,158.0f,(float)x+14,192.0f,(float)x+28,158.0f);
}

// =============================================================
//  FOG / MIST
// =============================================================
void drawFog(){
    if(fogAlpha <= 0.01f) return;
    for(int layer=0;layer<3;layer++){
        float yBase  = 162.0f + layer * 18.0f;
        float alpha  = fogAlpha * (1.0f - layer * 0.30f);
        alpha = clampf(alpha, 0.0f, 0.40f);
        float bright = 0.76f + 0.12f * sinf(waterShimmer + layer);
        float r = bright * alpha + (1.0f-alpha)*0.55f;
        float g = bright * alpha + (1.0f-alpha)*0.60f;
        float b = bright * alpha + (1.0f-alpha)*0.58f;
        glColor3f(r, g, b);
        for(int i=0;i<10;i++){
            float fx = i*125.0f + 25.0f*sinf(waterShimmer*0.25f + i*0.7f);
            float fw = 55.0f + 15.0f*sinf(waterShimmer*0.18f + i);
            float fh =  7.0f +  3.0f*sinf(waterShimmer*0.35f + i*1.1f);
            drawCircle(fx, yBase, fw, fh);
        }
    }
}

// =============================================================
//  CLOUDS
// =============================================================
void drawCloudAt(float cx,float cy,float scale){
    float tint=0.0f;
    if(timeOfDay>=0.65f&&timeOfDay<0.75f)       tint=(timeOfDay-0.65f)/0.10f;
    else if(timeOfDay>=0.75f&&timeOfDay<0.80f)  tint=1.0f-(timeOfDay-0.75f)/0.05f;
    else if(timeOfDay>=0.25f&&timeOfDay<0.35f)  tint=1.0f-(timeOfDay-0.25f)/0.10f;
    float r1=45.0f*scale,r2=52.0f*scale;
    float r1y=38.0f*scale,r2y=42.0f*scale,gap=50.0f*scale;
    glColor3f(1.0f,lerp(1.0f,0.55f,tint),lerp(1.0f,0.20f,tint));
    drawCircle(cx,       cy,              r1, r1y);
    drawCircle(cx+gap,   cy+18.0f*scale,  r2, r2y);
    drawCircle(cx+gap*2, cy,              r1, r1y);
    drawRect(cx,cy-r1y*0.5f,gap*2.0f,r1y*1.1f);
}

// =============================================================
//  AUTO BIRD
// =============================================================
void drawAutoBird(float bx,float by,float scale,float wAngle){
    glPushMatrix();
    glTranslatef(bx,by,0); glScalef(scale,scale,1.0f);
    glColor3f(0.04f,0.42f,0.08f); drawTriangle(-40,0,-58,12,-58,-12);
    glColor3f(0.10f,0.80f,0.20f); drawTriangle(-30,-14,22,0,-30,14);
    glPushMatrix(); glTranslatef(-10,6,0); glRotatef(wAngle,0,0,1);
    glColor3f(0.05f,0.58f,0.12f); drawTriangle(-18,0,6,0,-6,26);
    glPopMatrix();
    glColor3f(1.0f,0.50f,0.0f); drawTriangle(20,-5,42,0,20,5);
    glColor3f(0.05f,0.05f,0.05f); drawCircle(5,4,4,4);
    glColor3f(1.0f,1.0f,1.0f);   drawCircle(6,5,1.5f,1.5f);
    glPopMatrix();
}

// =============================================================
//  TOWER BIRD
// =============================================================
void drawTowerBird(){
    if(tBird.state==1 && getDayFactor()<0.1f) return;
    glPushMatrix();
    glTranslatef(tBird.x, tBird.y, 0);
    glScalef(0.55f, 0.55f, 1.0f);
    float wa = (tBird.state==1) ? 5.0f : tBird.wingAngle;
    glColor3f(0.55f,0.25f,0.05f); drawTriangle(-40,0,-58,12,-58,-12);
    glColor3f(0.80f,0.35f,0.05f); drawTriangle(-30,-14,22,0,-30,14);
    glPushMatrix(); glTranslatef(-10,6,0); glRotatef(wa,0,0,1);
    glColor3f(0.60f,0.28f,0.05f); drawTriangle(-18,0,6,0,-6,26);
    glPopMatrix();
    glColor3f(0.95f,0.70f,0.0f); drawTriangle(20,-5,38,0,20,5);
    glColor3f(0.05f,0.05f,0.05f); drawCircle(5,4,3.5f,3.5f);
    glColor3f(1.0f,1.0f,1.0f);   drawCircle(6,5,1.2f,1.2f);
    glPopMatrix();
}

// =============================================================
//  SMOKE
// =============================================================
void spawnSmoke(float x,float y){
    for(int i=0;i<MAX_SMOKE;i++){
        if(!smoke[i].active){
            smoke[i].x=x+((rand()%10)-5)*0.5f; smoke[i].y=y;
            smoke[i].vx=((rand()%20)-10)*0.05f; smoke[i].vy=0.8f+(rand()%10)*0.1f;
            smoke[i].life=1.0f; smoke[i].size=6.0f+(rand()%8);
            smoke[i].active=true; break;
        }
    }
}

void drawSmoke(){
    for(int i=0;i<MAX_SMOKE;i++){
        if(!smoke[i].active) continue;
        float g=0.5f+(1.0f-smoke[i].life)*0.4f;
        glColor3f(g,g,g);
        drawCircle(smoke[i].x,smoke[i].y,smoke[i].size*smoke[i].life,smoke[i].size*smoke[i].life);
    }
}

// =============================================================
//  LEAVES
// =============================================================
void spawnLeaf(float tx,float ty){
    for(int i=0;i<MAX_LEAVES;i++){
        if(!leaves[i].active){
            leaves[i].x      = tx + ((rand()%80)-40);
            leaves[i].y      = ty + ((rand()%60)-10);
            leaves[i].vx     = ((rand()%60)-10)*0.05f + 0.4f;
            leaves[i].vy     = ((rand()%30)-15)*0.05f;
            leaves[i].angle  = (float)(rand()%360);
            leaves[i].angleV = ((rand()%20)-10)*0.5f;
            leaves[i].life   = 1.0f;
            leaves[i].size   = 4.0f+(rand()%5);
            leaves[i].active = true;
            break;
        }
    }
}

void drawLeaves(){
    for(int i=0;i<MAX_LEAVES;i++){
        if(!leaves[i].active) continue;
        float a = leaves[i].life;
        glColor3f(0.20f+0.40f*(1.0f-a), 0.55f*a+0.30f*(1.0f-a), 0.05f);
        glPushMatrix();
        glTranslatef(leaves[i].x, leaves[i].y, 0);
        glRotatef(leaves[i].angle, 0, 0, 1);
        float s=leaves[i].size;
        drawTriangle(-s,0, 0,s*0.6f, s,0);
        glPopMatrix();
    }
}

// =============================================================
//  RAIN
// =============================================================
void drawRain(){
    if(!rainActive) return;
    glColor3f(0.55f,0.75f,0.95f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    for(int i=0;i<MAX_RAIN;i++){
        glVertex2f(rain[i].x,        rain[i].y);
        glVertex2f(rain[i].x+2.0f,   rain[i].y-rain[i].length);
    }
    glEnd();
    glLineWidth(1.0f);
}

// =============================================================
//  LIGHTNING FLASH
// =============================================================
void drawLightning(){
    if(lightningFlash <= 0.01f) return;
    glColor3f(lightningFlash, lightningFlash, lightningFlash);
    drawRect(0, 0, 1200, 700);
}

// =============================================================
//  STREET LAMPS  (FIXED)
//  - Larger, warmer glow cone that fans UPWARD from the bulb
//    so it's visible against dark backgrounds
//  - Soft multi-layer halo around the bulb head
//  - Uses glBlendFunc for additive-style blending on the halo
// =============================================================
void drawStreetLamps(){
    float nf = getNightFactor();
    float lampX[MAX_LAMPS] = { 155.0f, 390.0f, 720.0f, 1050.0f };

    // Enable blending for the glow layers
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(int i=0;i<MAX_LAMPS;i++){
        float lx = lampX[i];
        float ly = 160.0f;   // base on ground

        // ── Pole ──────────────────────────────────────────────────────
        glColor3f(0.38f,0.38f,0.40f);
        drawRect(lx-3, ly, 6, 110);

        // ── Horizontal arm extending right ────────────────────────────
        glColor3f(0.38f,0.38f,0.40f);
        drawRect(lx-3, ly+104, 24, 5);

        // ── Lamp head (housing) ───────────────────────────────────────
        glColor3f(0.28f,0.28f,0.30f);
        drawRect(lx+16, ly+100, 18, 13);

        // ── Night glow ────────────────────────────────────────────────
        if(nf > 0.05f){
            // Lamp flicker kept for the bulb only (subtle); halos are NOT flickered
            float bright = nf;

            // Warm sodium-lamp colour
            float br = clampf(1.00f * bright, 0.0f, 1.0f);
            float bg = clampf(0.82f * bright, 0.0f, 1.0f);
            float bb = clampf(0.22f * bright, 0.0f, 1.0f);

            // Lamp head centre (just below the housing)
            float hx = lx + 25.0f;
            float hy = ly + 107.0f;   // y ≈ 267

            // ── Halo rings – kept small so they never reach ground ────
            // Outer ring (radius 20 – stays well above y=247)
            glColor4f(br, bg * 0.65f, bb * 0.2f, 0.14f * bright);
            drawCircle(hx, hy, 20, 20);
            // Mid ring
            glColor4f(br, bg * 0.75f, bb * 0.3f, 0.28f * bright);
            drawCircle(hx, hy, 12, 12);
            // Inner ring
            glColor4f(br, bg * 0.88f, bb * 0.5f, 0.50f * bright);
            drawCircle(hx, hy,  7,  7);

            // ── Solid bright bulb core ────────────────────────────────
            glColor3f(br, bg, bb);
            drawCircle(hx, hy, 5, 5);

            // ── Downward light cone – short and narrow ────────────────
            //    apexY is the bottom face of the housing; cone fans downward.
            //    coneLen kept short (50px) so it stops at y≈210, above ground.
            float apexX    = hx;
            float apexY    = ly + 100.0f;   // y ≈ 260
            float coneHalfW = 32.0f;
            float coneLen   = 50.0f;        // reaches to y ≈ 210 — above children
            int   coneSteps = 20;

            glBegin(GL_TRIANGLE_FAN);
              glColor4f(br, bg * 0.70f, bb * 0.20f, 0.45f * bright);
              glVertex2f(apexX, apexY);
              for(int s = 0; s <= coneSteps; s++){
                  float t2  = (float)s / coneSteps;
                  float cx2 = apexX + (t2 - 0.5f) * coneHalfW * 2.0f;
                  float cy2 = apexY - coneLen;
                  glColor4f(br * 0.7f, bg * 0.50f, 0.0f, 0.0f); // fade to transparent
                  glVertex2f(cx2, cy2);
              }
            glEnd();
        }
    }

    glDisable(GL_BLEND);
}

// =============================================================
//  GOAL POST
// =============================================================
void drawGoalPost(){
    const float gx=635.0f,gy=162.0f,gw=130.0f,gh=72.0f,post=5.0f;
    glColor3f(0.92f,0.92f,0.92f);
    drawRect(gx,     gy,post,gh);
    drawRect(gx+gw,  gy,post,gh);
    drawRect(gx,gy+gh-post,gw+post,post);
    glColor3f(0.72f,0.72f,0.72f); glLineWidth(1.0f);
    glBegin(GL_LINES);
    for(int i=0;i<=7;i++){
        float nx=gx+post+i*(gw-post)/7.0f;
        glVertex2f(nx,gy); glVertex2f(nx,gy+gh-post);
    }
    for(int i=0;i<=4;i++){
        float ny=gy+i*(gh-post)/4.0f;
        glVertex2f(gx+post,ny); glVertex2f(gx+gw,ny);
    }
    glEnd();
    if(goalCelebration && sinf(goalTimer*8.0f)>0){
        float ts=7.0f,tx=540.0f,ty=235.0f;
        glColor3f(1.0f,0.88f,0.0f);
        drawRect(tx,ty,ts*4,ts); drawRect(tx,ty+ts,ts,ts*3);
        drawRect(tx+ts*2,ty+ts*2,ts*2,ts); drawRect(tx+ts*3,ty+ts,ts,ts*2);
        drawRect(tx,ty+ts*4,ts*4,ts); tx+=ts*6;
        drawRect(tx,ty,ts*4,ts); drawRect(tx,ty+ts,ts,ts*3);
        drawRect(tx+ts*3,ty+ts,ts,ts*3); drawRect(tx,ty+ts*4,ts*4,ts); tx+=ts*6;
        drawRect(tx+ts,ty,ts*2,ts); drawRect(tx,ty+ts,ts,ts*4);
        drawRect(tx+ts*3,ty+ts,ts,ts*4); drawRect(tx,ty+ts*2,ts*4,ts); tx+=ts*6;
        drawRect(tx,ty,ts,ts*4); drawRect(tx,ty+ts*4,ts*4,ts); tx+=ts*6;
        drawRect(tx+ts,ty,ts*2,ts*3); drawRect(tx+ts,ty+ts*4,ts*2,ts);
    }
}

// =============================================================
//  BUSH
// =============================================================
void drawBush(float bx,float by){
    glColor3f(0.05f,0.45f,0.05f);
    for(int i=0;i<6;i++){ float ox=bx+i*16.0f; drawTriangle(ox,by,ox+9,by+30,ox+18,by); }
    glColor3f(0.15f,0.68f,0.15f);
    for(int i=0;i<5;i++){ float ox=bx+8+i*16.0f; drawTriangle(ox,by,ox+9,by+24,ox+18,by); }
}

// =============================================================
//  PINE TREE
// =============================================================
void drawPineTree(float tx,float ty){
    glColor3f(0.52f,0.32f,0.08f); drawRect(tx-10,ty,20,65);
    glColor3f(0.08f,0.55f,0.08f); drawTriangle(tx-48,ty+55,tx,ty+135,tx+48,ty+55);
    glColor3f(0.12f,0.65f,0.12f); drawTriangle(tx-32,ty+98,tx,ty+168,tx+32,ty+98);
}

// =============================================================
//  ROUND TREE
// =============================================================
void drawRoundTree(float tx,float ty){
    glColor3f(0.48f,0.28f,0.06f); drawRect(tx-10,ty,20,72);
    glColor3f(0.03f,0.40f,0.03f); drawCircle(tx+8,ty+108,52,52);
    glColor3f(0.05f,0.55f,0.05f); drawCircle(tx,ty+110,52,52);
    glColor3f(0.20f,0.72f,0.20f); drawCircle(tx-10,ty+122,28,28);
}

// =============================================================
//  CLOCK TOWER
// =============================================================
void drawClockTower(){
    float nf=getNightFactor();
    float bx=260.0f,by=280.0f;
    glColor3f(0.55f,0.33f,0.12f); drawRect(bx,by,88,195);
    glColor3f(0.35f,0.18f,0.05f); drawRect(bx-6,by+178,100,14);
    glColor3f(0.88f,0.08f,0.08f); drawTriangle(bx-10,by+195,bx+44,by+265,bx+98,by+195);
    glColor3f(0.78f,0.78f,0.78f); drawCircle(bx+44,by+125,34,34);
    glColor3f(0.30f,0.30f,0.30f); drawCircle(bx+44,by+125,5,5);
    glColor3f(0.20f,0.20f,0.20f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(bx+44,by+125); glVertex2f(bx+44,by+148);
    glVertex2f(bx+44,by+125); glVertex2f(bx+62,by+130);
    glEnd(); glLineWidth(1.0f);
    if(nf>0){
        glColor3f(1.0f*nf, 0.88f*nf, 0.30f*nf);
    } else glColor3f(0.25f,0.12f,0.03f);
    drawRect(bx+30,by+35,28,35);
}

// =============================================================
//  WAREHOUSE
// =============================================================
void drawWarehouse(){
    float nf=getNightFactor();
    float bx=400.0f,by=280.0f;
    glColor3f(0.95f,0.65f,0.15f); drawRect(bx,by,320,165);
    glColor3f(0.80f,0.52f,0.10f); drawRect(bx,by+150,320,18);
    glColor3f(0.30f,0.14f,0.04f); drawRect(bx+112,by,96,118);
    glColor3f(0.50f,0.28f,0.08f); drawRect(bx+108,by+114,104,10);
    if(nf>0){
        glColor3f(1.0f*nf, 0.88f*nf, 0.30f*nf); drawRect(bx+25,by+80,55,45);
        glColor3f(1.0f*nf, 0.88f*nf, 0.30f*nf); drawRect(bx+240,by+80,55,45);
    } else {
        glColor3f(0.60f,0.80f,0.95f);
        drawRect(bx+25,by+80,55,45); drawRect(bx+240,by+80,55,45);
    }
    glColor3f(0.45f,0.60f,0.75f);
    drawRect(bx+50,by+80,4,45);   drawRect(bx+25,by+102,55,4);
    drawRect(bx+265,by+80,4,45);  drawRect(bx+240,by+102,55,4);
}

// =============================================================
//  SMALL HOUSE 
// =============================================================
void drawSmallHouse(){
    float nf=getNightFactor();
    float bx=850.0f,by=280.0f;
    glColor3f(0.92f,0.68f,0.28f); drawRect(bx,by,162,128);
    glColor3f(0.88f,0.08f,0.08f); drawTriangle(bx-12,by+128,bx+81,by+200,bx+174,by+128);
    glColor3f(0.65f,0.05f,0.05f); drawRect(bx+75,by+195,12,8);
    glColor3f(0.38f,0.20f,0.06f); drawRect(bx+46,by,70,82);
    glColor3f(0.50f,0.28f,0.10f);
    drawRect(bx+46,by+27,70,4); drawRect(bx+46,by+54,70,4); drawRect(bx+81,by,4,82);
    if(nf>0){
        glColor3f(1.0f*nf, 0.88f*nf, 0.30f*nf);
    } else glColor3f(0.60f,0.80f,0.95f);
    drawRect(bx+12,by+75,28,28);
    glColor3f(0.45f,0.60f,0.75f);
    drawRect(bx+25,by+75,3,28); drawRect(bx+12,by+89,28,3);
}

// =============================================================
//  RAILROAD TRACKS THE TRAIN WILLL RUN ON 
// =============================================================
void drawTracks(){
    glColor3f(0.42f,0.24f,0.08f);
    for(int x=0;x<1200;x+=38) drawRect((float)x,243,28,14);
    glColor3f(0.48f,0.14f,0.08f);
    drawRect(0,257,1200,9); drawRect(0,272,1200,9);
}

// =============================================================
//  TRAIN BODY STRUCTURE CODE 
// =============================================================
void drawTrain(){
    float nf=getNightFactor();
    glPushMatrix();
    glTranslatef(trainX,0,0);

    float yx=0.0f;
    glColor3f(0.95f,0.85f,0.05f); drawRect(yx,284,118,62);
    glColor3f(0.75f,0.65f,0.03f); drawRect(yx,340,118,8);
    glColor3f(0.80f,0.70f,0.03f);
    for(int i=1;i<4;i++) drawRect(yx+i*25,284,4,56);
    glColor3f(0.08f,0.08f,0.08f); drawCircle(yx+22,272,15,15); drawCircle(yx+95,272,15,15);
    glColor3f(0.40f,0.40f,0.40f); drawCircle(yx+22,272,6,6);   drawCircle(yx+95,272,6,6);
    glColor3f(0.25f,0.25f,0.25f); drawRect(yx+116,300,14,8);

    float rx=130.0f;
    glColor3f(0.88f,0.10f,0.10f); drawRect(rx,284,118,62);
    glColor3f(0.68f,0.06f,0.06f); drawRect(rx,340,118,8);
    if(nf>0) glColor3f(1.0f*nf,0.9f*nf,0.4f*nf);
    else     glColor3f(0.75f,0.90f,1.00f);
    drawRect(rx+18,305,35,25); drawRect(rx+65,305,35,25);
    glColor3f(0.55f,0.70f,0.85f);
    drawRect(rx+35,305,3,25); drawRect(rx+82,305,3,25);
    glColor3f(0.08f,0.08f,0.08f); drawCircle(rx+22,272,15,15); drawCircle(rx+95,272,15,15);
    glColor3f(0.40f,0.40f,0.40f); drawCircle(rx+22,272,6,6);   drawCircle(rx+95,272,6,6);
    glColor3f(0.25f,0.25f,0.25f); drawRect(rx+116,300,14,8);

    float lx=262.0f;
    glColor3f(0.50f,0.08f,0.72f); drawRect(lx,284,118,62);
    glColor3f(0.38f,0.05f,0.55f); drawRect(lx,346,52,30);
    glColor3f(0.28f,0.03f,0.40f); drawRect(lx+18,372,18,30);
    glColor3f(0.20f,0.02f,0.30f); drawRect(lx+14,400,26,8);
    glColor3f(0.75f,0.90f,1.00f); drawRect(lx+6,352,32,18);
    glColor3f(0.38f,0.05f,0.55f); drawRect(lx+112,290,10,38);
    if(nf>0 && trainSpeed>0.5f){
        glColor3f(1.0f*nf,1.0f*nf,0.7f*nf); drawCircle(lx+112,315,22,22);
    }
    glColor3f(1.00f,0.95f,0.50f); drawCircle(lx+112,315,8,8);
    glColor3f(0.58f,0.12f,0.80f); drawRect(lx+52,296,55,14);
    glColor3f(0.08f,0.08f,0.08f); drawCircle(lx+25,272,15,15); drawCircle(lx+90,272,15,15);
    glColor3f(0.40f,0.40f,0.40f); drawCircle(lx+25,272,6,6);   drawCircle(lx+90,272,6,6);
    glPopMatrix();
}

// =============================================================
//  PLAYER BIRD
// =============================================================
void drawBird(){
    glPushMatrix();
    glTranslatef(bird.x,bird.y,0);
    glColor3f(0.04f,0.42f,0.08f); drawTriangle(-40,0,-58,12,-58,-12);
    glColor3f(0.10f,0.80f,0.20f); drawTriangle(-30,-14,22,0,-30,14);
    glPushMatrix(); glTranslatef(-10,6,0); glRotatef(bird.wingAngle,0,0,1);
    glColor3f(0.05f,0.58f,0.12f); drawTriangle(-18,0,6,0,-6,26);
    glPopMatrix();
    glColor3f(1.0f,0.50f,0.0f); drawTriangle(20,-5,42,0,20,5);
    glColor3f(0.05f,0.05f,0.05f); drawCircle(5,4,4,4);
    glColor3f(1.0f,1.0f,1.0f);   drawCircle(6,5,1.5f,1.5f);
    glPopMatrix();
}

// =============================================================
//  HUD
// =============================================================
void drawHUD(){
    char buf[64];
    sprintf(buf,"SCORE: %d",bird.score);
    glColor3f(1.0f,1.0f,1.0f); glRasterPos2f(10,682);
    for(int i=0;buf[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,buf[i]);
    sprintf(buf,"FPS: %.0f",fps);
    glRasterPos2f(10,662);
    for(int i=0;buf[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,buf[i]);
    const char* tod;
    if(timeOfDay>=0.85f||timeOfDay<0.15f)             tod="NIGHT";
    else if(timeOfDay>=0.25f&&timeOfDay<0.35f)        tod="SUNRISE";
    else if(timeOfDay>=0.35f&&timeOfDay<0.65f)        tod="DAY";
    else if(timeOfDay>=0.65f&&timeOfDay<0.75f)        tod="SUNSET";
    else                                               tod="DUSK/DAWN";
    glRasterPos2f(10,642);
    for(int i=0;tod[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,tod[i]);
    glColor3f(0.90f,0.90f,0.90f);
    const char* h1="Arrow keys: Move Bird  |  T: Train  |  R: Rain  |  P: Pause Time  |  ESC: Quit";
    glRasterPos2f(160,682);
    for(int i=0;h1[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,h1[i]);
    if(rainActive){
        glColor3f(0.50f,0.82f,1.0f); glRasterPos2f(10,622);
        const char* rl="RAIN ON";
        for(int i=0;rl[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,rl[i]);
    }
    if(trainMoving||trainSpeed>0){
        glColor3f(1.0f,0.90f,0.40f); glRasterPos2f(10,602);
        const char* tl=trainMoving?"TRAIN RUNNING":"TRAIN STOPPING";
        for(int i=0;tl[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,tl[i]);
    }
}

// =============================================================
//  FOOTBALL BASIXC BALL 
// =============================================================
void drawFootball(float fx,float fy){
    float r=11.0f;
    glColor3f(1.0f,1.0f,1.0f); drawCircle(fx,fy,r,r);
    glColor3f(0.0f,0.0f,0.0f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<=360;i++){ float a=i*PI/180.0f; glVertex2f(fx+r*cosf(a),fy+r*sinf(a)); }
    glEnd();
    glPushMatrix(); glTranslatef(fx,fy,0); glRotatef(ballSpin,0,0,1);
    glColor3f(0.05f,0.05f,0.05f);
    for(int i=0;i<5;i++){
        float a=i*(2.0f*PI/5.0f);
        glBegin(GL_LINES); glVertex2f(0,0); glVertex2f((r-2.0f)*cosf(a),(r-2.0f)*sinf(a)); glEnd();
    }
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<5;i++){ float a=i*(2.0f*PI/5.0f); glVertex2f((r-3.5f)*cosf(a),(r-3.5f)*sinf(a)); }
    glEnd();
    glLineWidth(1.0f); glPopMatrix();
}

// =============================================================
//  CHILDREN PLAYING IN THE FIELD ALL THE THEIR BODEY PART 
// =============================================================
void drawChild(float x,float ground,
               float sr,float sg,float sb,
               float pr,float pg,float pb,
               int pose){
    float hy=ground, kny=ground+18.0f, hip=ground+34.0f;
    float sho=ground+56.0f, nky=ground+62.0f, hcy=ground+74.0f;
    float hr=10.0f;

    glColor3f(pr,pg,pb); glLineWidth(4.5f);
    if(pose==0){
        glBegin(GL_LINES);
        glVertex2f(x-5,hip); glVertex2f(x-14,kny); glVertex2f(x-14,kny); glVertex2f(x-8,hy);
        glVertex2f(x+5,hip); glVertex2f(x+8,kny);  glVertex2f(x+8,kny);  glVertex2f(x+16,hy);
        glEnd();
    } else if(pose==1){
        glBegin(GL_LINES);
        glVertex2f(x-5,hip); glVertex2f(x-6,kny);     glVertex2f(x-6,kny);    glVertex2f(x-4,hy);
        glVertex2f(x+5,hip); glVertex2f(x+18,kny+10); glVertex2f(x+18,kny+10);glVertex2f(x+30,kny+6);
        glEnd();
    } else if(pose==2){
        glBegin(GL_LINES);
        glVertex2f(x-5,hip); glVertex2f(x-16,kny); glVertex2f(x-16,kny); glVertex2f(x-12,hy);
        glVertex2f(x+5,hip); glVertex2f(x+16,kny); glVertex2f(x+16,kny); glVertex2f(x+12,hy);
        glEnd();
    } else if(pose==3){
        glBegin(GL_LINES);
        glVertex2f(x+5,hip);  glVertex2f(x+14,kny); glVertex2f(x+14,kny); glVertex2f(x+8,hy);
        glVertex2f(x-5,hip);  glVertex2f(x-8,kny);  glVertex2f(x-8,kny);  glVertex2f(x-16,hy);
        glEnd();
    } else {
        glBegin(GL_LINES);
        glVertex2f(x-5,hip); glVertex2f(x-20,kny); glVertex2f(x-20,kny); glVertex2f(x-18,hy);
        glVertex2f(x+5,hip); glVertex2f(x+20,kny); glVertex2f(x+20,kny); glVertex2f(x+18,hy);
        glEnd();
    }

    glColor3f(0.15f,0.08f,0.02f); glLineWidth(5.5f);
    if(pose==0){
        glBegin(GL_LINES); glVertex2f(x-8,hy); glVertex2f(x-18,hy); glVertex2f(x+16,hy); glVertex2f(x+6,hy); glEnd();
    } else if(pose==1){
        glBegin(GL_LINES); glVertex2f(x-4,hy); glVertex2f(x-14,hy); glVertex2f(x+30,kny+6); glVertex2f(x+40,kny+2); glEnd();
    } else if(pose==2){
        glBegin(GL_LINES); glVertex2f(x-12,hy); glVertex2f(x-22,hy); glVertex2f(x+12,hy); glVertex2f(x+22,hy); glEnd();
    } else if(pose==3){
        glBegin(GL_LINES); glVertex2f(x+8,hy); glVertex2f(x+18,hy); glVertex2f(x-16,hy); glVertex2f(x-6,hy); glEnd();
    } else {
        glBegin(GL_LINES); glVertex2f(x-18,hy); glVertex2f(x-28,hy); glVertex2f(x+18,hy); glVertex2f(x+28,hy); glEnd();
    }
    glLineWidth(1.0f);

    glColor3f(sr,sg,sb); drawRect(x-10,hip,20,sho-hip);

    glColor3f(0.82f,0.62f,0.48f); glLineWidth(4.0f);
    if(pose==0){
        glBegin(GL_LINES); glVertex2f(x-10,sho-4); glVertex2f(x-24,sho-18); glVertex2f(x+10,sho-4); glVertex2f(x+18,sho-22); glEnd();
    } else if(pose==1){
        glBegin(GL_LINES); glVertex2f(x-10,sho-4); glVertex2f(x-26,sho-10); glVertex2f(x+10,sho-4); glVertex2f(x+20,sho-8); glEnd();
    } else if(pose==2){
        glBegin(GL_LINES); glVertex2f(x-10,sho); glVertex2f(x-24,sho+22); glVertex2f(x+10,sho); glVertex2f(x+24,sho+22); glEnd();
    } else if(pose==3){
        glBegin(GL_LINES); glVertex2f(x+10,sho-4); glVertex2f(x+24,sho-18); glVertex2f(x-10,sho-4); glVertex2f(x-18,sho-22); glEnd();
    } else {
        glBegin(GL_LINES); glVertex2f(x-10,sho-2); glVertex2f(x-34,sho+5); glVertex2f(x+10,sho-2); glVertex2f(x+34,sho+5); glEnd();
    }
    glLineWidth(1.0f);

    glColor3f(0.82f,0.62f,0.48f);
    drawRect(x-4,nky,8,hcy-nky-2);
    drawCircle(x,hcy,hr,hr);
    glColor3f(0.25f,0.15f,0.05f); drawCircle(x,hcy+2,hr,hr*0.55f);
    glColor3f(0.10f,0.10f,0.10f);
    drawCircle(x-3.5f,hcy+1,2.2f,2.2f); drawCircle(x+3.5f,hcy+1,2.2f,2.2f);
    glColor3f(0.55f,0.20f,0.10f); glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=8;i++){
        float a=PI+i*(PI/8.0f);
        glVertex2f(x+5.0f*cosf(a),hcy-2.0f+3.5f*sinf(a));
    }
    glEnd(); glLineWidth(1.0f);
}

void drawChildren(){
    float gnd=162.0f;
    for(int i=0;i<5;i++){
        int pose;
        if(i==4){
            pose=4;
        } else if(fabsf(children[i].speed)<0.05f){
            pose=children[i].basepose;
        } else {
            pose=(children[i].targetX > children[i].x) ? 3 : 0;
        }
        drawChild(children[i].x, gnd,
                  children[i].sr,children[i].sg,children[i].sb,
                  children[i].pr,children[i].pg,children[i].pb,
                  pose);
    }
    if(fabsf(children[0].x - ballX) < 25.0f)
        drawChild(children[0].x,gnd,
                  children[0].sr,children[0].sg,children[0].sb,
                  children[0].pr,children[0].pg,children[0].pb, 1);
    drawFootball(ballX,gnd+11.0f);
}

// =============================================================
//  All the display function 
// =============================================================
void display(){
    float sr,sg,sb;
    getSkyColor(timeOfDay,sr,sg,sb,false);
    glClearColor(sr,sg,sb,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawSky();
    drawStars();
    drawMoon();
    drawLeftSun();
    drawSun();

    for(int i=0;i<3;i++) drawCloudAt(clouds[i].x,clouds[i].y,clouds[i].scale);
    for(int i=0;i<3;i++) drawAutoBird(autoBirds[i].x,autoBirds[i].y,autoBirds[i].scale,autoBirds[i].wingAngle);

    drawPineTree(195,295); drawPineTree(760,290);
    drawRoundTree(1055,295); drawRoundTree(1148,290);

    drawClockTower(); drawWarehouse(); drawSmallHouse();

    drawBush(388,278); drawBush(716,278);
    drawBush(840,278); drawBush(1010,278);

    drawGround();
    // drawRiver() REMOVED
    drawTracks();
    drawTrain();
    drawSmoke();
    drawBottomGrass();
    drawLeaves();
    drawStreetLamps();
    drawGoalPost();
    drawChildren();
    drawRain();
    drawFog();
    drawTowerBird();
    drawBird();
    drawLightning();
    drawHUD();

    glutSwapBuffers();

    frameCount++;
    int now=glutGet(GLUT_ELAPSED_TIME);
    if(now-lastFPSTime>500){
        fps=frameCount*1000.0f/(now-lastFPSTime);
        frameCount=0; lastFPSTime=now;
    }
}

// =============================================================
//  RESHAPE
// =============================================================
void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0,WINDOW_W,0,WINDOW_H,-1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

// =============================================================
//  TIMER  (~60 fps)
// =============================================================
void timer(int){
    if(!pauseTime){
        timeOfDay+=daySpeed;
        if(timeOfDay>=1.0f) timeOfDay-=1.0f;
        for(int i=0;i<MAX_STARS;i++) stars[i].twinklePhase+=stars[i].twinkleSpeed;
    }

    waterShimmer += 0.04f;

    float targetFog = getDuskFactor() * 0.45f;
    fogAlpha += (targetFog - fogAlpha) * 0.008f;

    const float ACC=0.50f, MAX_V=8.0f;
    if(keyDown[0]){ bird.vx-=ACC; if(bird.vx<-MAX_V) bird.vx=-MAX_V; }
    if(keyDown[1]){ bird.vx+=ACC; if(bird.vx> MAX_V) bird.vx= MAX_V; }
    if(keyDown[2]){ bird.vy+=ACC; if(bird.vy> MAX_V) bird.vy= MAX_V; }
    if(keyDown[3]){ bird.vy-=ACC; if(bird.vy<-MAX_V) bird.vy=-MAX_V; }
    bird.x+=bird.vx; bird.y+=bird.vy;
    if(bird.x<  30) bird.x=  30; if(bird.x>1170) bird.x=1170;
    if(bird.y< 200) bird.y= 200; if(bird.y> 690) bird.y= 690;
    bird.wingAngle+=bird.wingDir*4.5f;
    if(bird.wingAngle> 22) bird.wingDir=-1;
    if(bird.wingAngle<-22) bird.wingDir= 1;

    if(trainMoving){ trainSpeed+=0.08f; if(trainSpeed>TRAIN_MAX) trainSpeed=TRAIN_MAX; }
    else           { trainSpeed-=0.08f; if(trainSpeed<0) trainSpeed=0; }
    if(trainSpeed>0){
        trainX+=trainSpeed;
        if(trainX>1420) trainX=-430.0f;
        static float lastSmokeX=0;
        if(fabsf(trainX-lastSmokeX)>8.0f){
            spawnSmoke(trainX+262+18+9,408);
            lastSmokeX=trainX;
        }
    }

    for(int i=0;i<MAX_SMOKE;i++){
        if(!smoke[i].active) continue;
        smoke[i].x+=smoke[i].vx; smoke[i].y+=smoke[i].vy;
        smoke[i].life-=0.015f; smoke[i].size+=0.30f;
        if(smoke[i].life<=0) smoke[i].active=false;
    }

    ballX+=ballVX;
    ballSpin+=(ballVX>0?4.0f:-4.0f);
    if(ballX>960.0f) ballVX=-1.8f;
    if(ballX<210.0f) ballVX= 1.8f;

    if(!goalCelebration && ballX>635.0f && ballX<765.0f){
        goalCelebration=true; goalTimer=0.0f; bird.score++;
    }
    if(goalCelebration){ goalTimer+=0.016f; if(goalTimer>2.5f) goalCelebration=false; }

    for(int i=0;i<4;i++){
        children[i].targetX = ballX + (i%2==0 ? -40.0f : 40.0f);
        if(children[i].targetX<160.0f)  children[i].targetX=160.0f;
        if(children[i].targetX>1120.0f) children[i].targetX=1120.0f;
        float dx = children[i].targetX - children[i].x;
        if(fabsf(dx)>2.0f){
            float step = (dx>0?1.0f:-1.0f) * children[i].speed;
            children[i].x += step;
        }
        children[i].animTimer += 0.08f;
    }

    for(int i=0;i<3;i++){
        clouds[i].x-=clouds[i].speed;
        if(clouds[i].x+200.0f*clouds[i].scale<0) clouds[i].x=1220.0f;
    }

    for(int i=0;i<3;i++){
        autoBirds[i].x+=autoBirds[i].speed;
        if(autoBirds[i].x>1260.0f) autoBirds[i].x=-80.0f;
        autoBirds[i].wingAngle+=autoBirds[i].wingDir*5.0f;
        if(autoBirds[i].wingAngle> 22) autoBirds[i].wingDir=-1;
        if(autoBirds[i].wingAngle<-22) autoBirds[i].wingDir= 1;
    }

    if(tBird.state==0){
        float dx=TOWER_SEAT_X-tBird.x, dy=TOWER_SEAT_Y-tBird.y;
        float dist=sqrtf(dx*dx+dy*dy);
        if(dist<3.0f){
            tBird.x=TOWER_SEAT_X; tBird.y=TOWER_SEAT_Y;
            tBird.vx=0; tBird.vy=0;
            tBird.state=1; tBird.seatTimer=0.0f;
        } else {
            tBird.vx=dx/dist*1.6f; tBird.vy=dy/dist*1.2f;
            tBird.x+=tBird.vx; tBird.y+=tBird.vy;
        }
        tBird.wingAngle+=tBird.wingDir*5.0f;
        if(tBird.wingAngle> 22) tBird.wingDir=-1;
        if(tBird.wingAngle<-22) tBird.wingDir= 1;
    } else if(tBird.state==1){
        tBird.seatTimer+=0.016f;
        tBird.wingAngle=5.0f;
        if(tBird.seatTimer>6.0f) tBird.state=2;
    } else {
        tBird.vx+=0.04f; tBird.vy+=0.015f;
        tBird.x+=tBird.vx; tBird.y+=tBird.vy;
        tBird.wingAngle+=tBird.wingDir*5.0f;
        if(tBird.wingAngle> 22) tBird.wingDir=-1;
        if(tBird.wingAngle<-22) tBird.wingDir= 1;
        if(tBird.x>1280.0f){
            tBird.x=-60.0f;
            tBird.y=440.0f+(float)(rand()%50);
            tBird.vx=1.4f;  tBird.vy=0.0f;
            tBird.state=0;
        }
    }

    static int leafCounter=0;
    leafCounter++;
    if(leafCounter%22==0) spawnLeaf(1060.0f, 405.0f);
    if(leafCounter%31==0) spawnLeaf(1153.0f, 402.0f);

    for(int i=0;i<MAX_LEAVES;i++){
        if(!leaves[i].active) continue;
        leaves[i].x     +=leaves[i].vx;
        leaves[i].y     +=leaves[i].vy;
        leaves[i].vy    -=0.04f;
        leaves[i].vx    +=0.01f;
        leaves[i].angle +=leaves[i].angleV;
        leaves[i].life  -=0.006f;
        if(leaves[i].life<=0||leaves[i].y<0) leaves[i].active=false;
    }

    for(int i=0;i<MAX_LAMPS;i++) lampFlicker[i]+=0.07f+(float)i*0.013f;
    for(int i=0;i<4;i++)         winFlicker[i] +=0.05f+(float)i*0.009f;

    if(rainActive){
        for(int i=0;i<MAX_RAIN;i++){
            rain[i].y-=rain[i].speed; rain[i].x+=1.5f;
            if(rain[i].y<0){ rain[i].y=700.0f+(float)(rand()%100); rain[i].x=(float)(rand()%1200); }
        }
    }

    if(rainActive){
        lightningTimer-=0.016f;
        if(lightningTimer<=0.0f){
            lightningFlash=0.85f;
            lightningInterval=5.0f+(float)(rand()%40)*0.1f;
            lightningTimer=lightningInterval;
        }
    } else {
        lightningTimer=3.0f;
    }
    if(lightningFlash>0.0f){
        lightningFlash-=0.30f;
        if(lightningFlash<0.0f) lightningFlash=0.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16,timer,0);
}

// =============================================================
//  input taking part 
// =============================================================
void specialKeyDown(int key,int,int){
    if(key==GLUT_KEY_LEFT)  keyDown[0]=true;
    if(key==GLUT_KEY_RIGHT) keyDown[1]=true;
    if(key==GLUT_KEY_UP)    keyDown[2]=true;
    if(key==GLUT_KEY_DOWN)  keyDown[3]=true;
}

void specialKeyUp(int key,int,int){
    if(key==GLUT_KEY_LEFT)  { keyDown[0]=false; bird.vx=0; }
    if(key==GLUT_KEY_RIGHT) { keyDown[1]=false; bird.vx=0; }
    if(key==GLUT_KEY_UP)    { keyDown[2]=false; bird.vy=0; }
    if(key==GLUT_KEY_DOWN)  { keyDown[3]=false; bird.vy=0; }
}

void keyboard(unsigned char key,int,int){
    switch(key){
        case 't': case 'T': trainMoving=!trainMoving; break;
        case 'r': case 'R': rainActive =!rainActive;  break;
        case 'p': case 'P': pauseTime  =!pauseTime;   break;
        case 27: exit(0);
    }
}

// =============================================================
//  This is main function part 
// =============================================================
int main(int argc,char** argv){
    srand(42);
    initSmoke();
    initRain();
    initStars();
    initLeaves();
    initFlicker();

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WINDOW_W,WINDOW_H);
    glutInitWindowPosition(80,40);
    glutCreateWindow("Animated Scene - Enhanced v8");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);  // off by default; enabled per-draw in lamp function

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16,timer,0);

    lastFPSTime=glutGet(GLUT_ELAPSED_TIME);
    glutMainLoop();
    return 0;
}
