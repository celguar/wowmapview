#ifndef _NODES_H
#define _NODES_H

#include <vector>
#include <string>
#include "vec3d.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "video.h"
#include "quaternion.h"
#include "defines/Common.h"
#include "model.h"

struct TravelNode {
    uint32 id;
    std::string name;
    uint32 mapId;
    float x;
    float y;
    float z;
    uint8 linked;

    Vec3D position;
    float radius;
};

class WorldBotNodes {
public:
    static const float DEFAULT_BOX_SIZE;

    void LoadFromDB();
    void Draw(int mapId);

    std::vector<TravelNode> nodes;

private:
    GLuint sphereDisplayList;

    void DrawBox(const Vec3D& pos, float size, const Vec4D& color);

    Model* nodeModel;

};

#endif