#ifndef MAPTILE_H
#define MAPTILE_H

#define TILESIZE (533.33333f)
#define CHUNKSIZE ((TILESIZE) / 16.0f)
#define UNITSIZE (CHUNKSIZE / 8.0f)
#define ZEROPOINT (32.0f * (TILESIZE))

#include "video.h"
#include "mpq.h"
#include "wmo.h"
#include "model.h"
#include "liquid.h"
#include <vector>
#include <string>

class MapTile;
class MapChunk;

class World;

const int mapbufsize = 9*9 + 8*8;

class MapNode {
public:

    MapNode(int x, int y, int s):px(x),py(y),size(s) {}

    int px, py, size;

    Vec3D vmin, vmax, vcenter;

    MapNode *children[4];
    MapTile *mt;

    virtual void draw();
    void setup(MapTile *t);
    void cleanup();

};

class MapChunk : public MapNode {
public:
    int nTextures;

    float xbase, ybase, zbase;
    float r;

    unsigned int areaID;

    bool haswater;
    bool visible;
    bool hasholes;
    float waterlevel;

    TextureID textures[4];
    TextureID alphamaps[3];
    TextureID shadow, blend;

    int animated[4];

    GLuint vertices, normals;

    short *strip;
    int striplen;

    Liquid *lq;

    MapChunk():MapNode(0,0,0) {}

    void init(MapTile* mt, MPQFile &f);
    void destroy();
    void initStrip(int holes);

    void draw();
    void drawNoDetail();
    void drawPass(int anim);
    void drawWater();

};

class MapTile {
public:
    std::vector<std::string> textures;
    std::vector<std::string> wmos;
    std::vector<std::string> models;

    std::vector<WMOInstance> wmois;
    std::vector<ModelInstance> modelis;
    int nWMO;
    int nMDX;

    int x, z;
    bool ok;

    //World *world;

    float xbase, zbase;

    MapChunk chunks[16][16];

    MapNode topnode;

    MapTile(int x0, int z0, char* filename);
    ~MapTile();

    void draw();
    void drawWater();
    void drawObjects();
    void drawSky();
    //void drawPortals();
    void drawModels();

    /// Get chunk for sub offset x,z
    MapChunk *getChunk(unsigned int x, unsigned int z);
};

int indexMapBuf(int x, int y);


// 8x8x2 version with triangle strips, size = 8*18 + 7*2
const int stripsize = 8*18 + 7*2;
template <class V>
void stripify(V *in, V *out)
{
    for (int row=0; row<8; row++) {
        V *thisrow = &in[indexMapBuf(0,row*2)];
        V *nextrow = &in[indexMapBuf(0,(row+1)*2)];

        if (row>0) *out++ = thisrow[0];
        for (int col=0; col<9; col++) {
            *out++ = thisrow[col];
            *out++ = nextrow[col];
        }
        if (row<7) *out++ = nextrow[8];
    }
}

// high res version, size = 16*18 + 7*2 + 8*2
const int stripsize2 = 16*18 + 7*2 + 8*2;
template <class V>
void stripify2(V *in, V *out)
{
    for (int row=0; row<8; row++) {
        V *thisrow = &in[indexMapBuf(0,row*2)];
        V *nextrow = &in[indexMapBuf(0,row*2+1)];
        V *overrow = &in[indexMapBuf(0,(row+1)*2)];

        if (row>0) *out++ = thisrow[0];// jump end
        for (int col=0; col<8; col++) {
            *out++ = thisrow[col];
            *out++ = nextrow[col];
        }
        *out++ = thisrow[8];
        *out++ = overrow[8];
        *out++ = overrow[8];// jump start
        *out++ = thisrow[0];// jump end
        *out++ = thisrow[0];
        for (int col=0; col<8; col++) {
            *out++ = overrow[col];
            *out++ = nextrow[col];
        }
        if (row<8) *out++ = overrow[8];
        if (row<7) *out++ = overrow[8];// jump start
    }
}


#endif
