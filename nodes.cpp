#include "nodes.h"
#include "wowmapview.h"
#include "world.h"
#include "Database/Database.h"
#include <cmath>
#include <algorithm>
#include <SDL.h>
#include <SDL_opengl.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

extern Database GameDb;

const float WorldBotNodes::DEFAULT_BOX_SIZE = 3.0f;  // Make box 3x3x3 units

void WorldBotNodes::LoadFromDB()
{
    nodes.clear();

    std::string query = "SELECT id, name, map_id, x, y, z, linked FROM ai_playerbot_travelnode ORDER BY id";
    auto result = GameDb.Query(query.c_str());
    if (!result)
        return;

    do {
        DbField* fields = result->fetchCurrentRow();

        TravelNode node;
        node.id = fields[0].GetUInt32();
        node.name = fields[1].GetString();
        node.mapId = fields[2].GetUInt32();
        node.x = fields[3].GetFloat();
        node.y = fields[4].GetFloat();
        node.z = fields[5].GetFloat();
        node.linked = fields[6].GetUInt8();

        if (node.mapId != gWorld->currentMapId)
            continue;

		gLog("Loaded travel node %u: %s (%.2f, %.2f, %.2f) on map %u\n", node.id, node.name.c_str(), node.x, node.y, node.z, node.mapId);

        // Convert WoW coordinates to OpenGL coordinates
		node.position = Vec3D(-(node.y - ZEROPOINT), (node.z), -(node.x - ZEROPOINT));
        node.radius = 2.0f;

        nodes.push_back(node);

    } while (result->NextRow());
}

void WorldBotNodes::DrawBox(const Vec3D& pos, float size, const Vec4D& color)
{
    float half = size / 2.0f;

    // Set the color with transparency
    glColor4f(color.x, color.y, color.z, color.w);

    glBegin(GL_QUADS);

    // Front
    glVertex3f(pos.x - half, pos.y - half, pos.z + half);
    glVertex3f(pos.x + half, pos.y - half, pos.z + half);
    glVertex3f(pos.x + half, pos.y + half, pos.z + half);
    glVertex3f(pos.x - half, pos.y + half, pos.z + half);

    // Back
    glVertex3f(pos.x - half, pos.y - half, pos.z - half);
    glVertex3f(pos.x - half, pos.y + half, pos.z - half);
    glVertex3f(pos.x + half, pos.y + half, pos.z - half);
    glVertex3f(pos.x + half, pos.y - half, pos.z - half);

    // Top
    glVertex3f(pos.x - half, pos.y + half, pos.z - half);
    glVertex3f(pos.x - half, pos.y + half, pos.z + half);
    glVertex3f(pos.x + half, pos.y + half, pos.z + half);
    glVertex3f(pos.x + half, pos.y + half, pos.z - half);

    // Bottom
    glVertex3f(pos.x - half, pos.y - half, pos.z - half);
    glVertex3f(pos.x + half, pos.y - half, pos.z - half);
    glVertex3f(pos.x + half, pos.y - half, pos.z + half);
    glVertex3f(pos.x - half, pos.y - half, pos.z + half);

    // Right
    glVertex3f(pos.x + half, pos.y - half, pos.z - half);
    glVertex3f(pos.x + half, pos.y + half, pos.z - half);
    glVertex3f(pos.x + half, pos.y + half, pos.z + half);
    glVertex3f(pos.x + half, pos.y - half, pos.z + half);

    // Left
    glVertex3f(pos.x - half, pos.y - half, pos.z - half);
    glVertex3f(pos.x - half, pos.y - half, pos.z + half);
    glVertex3f(pos.x - half, pos.y + half, pos.z + half);
    glVertex3f(pos.x - half, pos.y + half, pos.z - half);

    glEnd();
}

void WorldBotNodes::Draw(int mapId)
{
    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable texturing for solid colors
    glDisable(GL_TEXTURE_2D);

    // Enable depth testing but disable depth writing for transparency
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Draw all boxes
    for (const auto& node : nodes)
    {
        if (node.mapId != mapId)
            continue;

        // Choose color based on linked status - use high alpha for visibility
        Vec4D color = node.linked ?
            Vec4D(0.0f, 1.0f, 0.0f, 0.7f) :  // Green for linked
            Vec4D(1.0f, 0.0f, 0.0f, 0.7f);   // Red for unlinked

        DrawBox(node.position, DEFAULT_BOX_SIZE, color);
    }

    // Reset states
    glDepthMask(GL_TRUE);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
