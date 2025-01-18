#include "test.h"
#include "wowmapview.h"
#include "areadb.h"
#include "shaders.h"

#include <cmath>
#include <string>

using namespace std;


//#define XSENS 15.0f
//#define YSENS 15.0f
#define XSENS 4.0f
#define YSENS 8.0f
#define SPEED 66.6f

// why the hell is this class called Test, anyway
// I should rename it to MapViewer or something when I'm not lazy

Test::Test(World *w, float ah0, float av0): world(w), ah(ah0), av(av0)
{
	moving = strafing = updown = 0;

	mousedir = -1.0f;

	movespd = SPEED;

	look = false;
	mapmode = false;
	hud = true;

	world->thirdperson = false;
	world->lighting = true;
	world->drawmodels = true;
	world->drawdoodads = true;
	world->drawterrain = true;
	world->drawwmo = true;
	world->drawhighres = true;
	world->drawfog = true; // should this be on or off by default..? :(

	// in the wow client, fog distance is stored in wtf\config.wtf as "farclip"
	// minimum is 357, maximum is 777
	world->fogdistance = 512.0f;

	world->l_const = 0.0f;
	world->l_linear = 0.7f;
	world->l_quadratic = 0.03f;
}

Test::~Test()
{
	delete world;
}

void Test::tick(float t, float dt)
{
	Vec3D dir(1, 0, 0);
	rotate(0, 0, &dir.x, &dir.y, av * PI / 180.0f);
	rotate(0, 0, &dir.x, &dir.z, ah * PI / 180.0f);

	// Updates camera and lookat based on movement
	if (moving != 0) world->camera += dir * dt * movespd * moving;
	if (strafing != 0) {
		Vec3D right = dir % Vec3D(0, 1, 0);
		right.normalize();
		world->camera += right * dt * movespd * strafing;
	}
	if (updown != 0) world->camera += Vec3D(0, dt * movespd * updown, 0);
	world->lookat = world->camera + dir;

	// Updates world time and animations
	world->time += (world->modelmanager.v * 90.0f * dt);
	world->animtime += dt * 1000.0f;
	globalTime = (int)world->animtime;

	// Updates world state
	world->tick(dt);
}

void Test::display(float t, float dt)
{
	if (mapmode && world->minimap) {
		// show map
        // TODO: try to use a real map from WoW? either the large map or the minimap would be nice
		video.clearScreen();
		video.set2D();

		const int len = 768;
		const int basex = 200;
		const int basey = 0;
		glColor4f(1,1,1,1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, world->minimap);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2i(basex,basey);
		glTexCoord2f(1,0);
		glVertex2i(basex+len,basey);
		glTexCoord2f(1,1);
		glVertex2i(basex+len,basey+len);
		glTexCoord2f(0,1);
		glVertex2i(basex,basey+len);
		glEnd();

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		float fx, fz;
		fx = basex + world->camera.x / TILESIZE * 12.0f;
		fz = basey + world->camera.z / TILESIZE * 12.0f;
		glVertex2f(fx, fz);
		glColor4f(1,1,1,0);
		glVertex2f(fx + 10.0f*cosf(ah/180.0f*PI), fz + 10.0f*sinf(ah/180.0f*PI));
		glEnd();
	} else {
        // draw 3D view
		video.set3D();
		world->draw();
		
		video.set2D();
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		glColor4f(1,1,1,1);

		if (hud) {
			f16->print(5,0,"%.2f fps", gFPS);

			//char *sn = world->skies->getSkyName();
			//if (sn)	f16->print(5,60,"%s", sn);

			// TODO: look up WMO names/group names as well from some client db?
			unsigned int areaID = world->getAreaID();
			unsigned int regionID = 0;
			/// Look up area
			try {
				AreaDB::Record rec = gAreaDB.getByAreaID(areaID);
				std::string areaName = rec.getString(AreaDB::Name);
				regionID = rec.getUInt(AreaDB::Region);
				f16->print(5,20,"%s", areaName.c_str());
			} catch(AreaDB::NotFound)
			{
				/// Not found, unknown area
				//f16->print(5,20,"Unknown [%i]", areaID);
			}
			if (regionID != 0) {
				/// Look up region
				try {
					AreaDB::Record rec = gAreaDB.getByAreaID(regionID);
					std::string regionName = rec.getString(AreaDB::Name);
					f16->print(5,40,"%s", regionName.c_str());
				} catch(AreaDB::NotFound)
				{
					//f16->print(5,40,"Unknown [%i]", regionID);
				}
			}

			//f16->print(5,60,"%d", world->modelmanager.v);

			int time = ((int)world->time)%2880;
			int hh,mm;

            hh = time / 120;
			mm = (time % 120) / 2;
			
			//f16->print(5, 60, "%02d:%02d", hh,mm);
			f16->print(video.xres - 50, 0, "%02d:%02d", hh,mm);

			// Show the raw camera coordinates								
			//f16->print(5, video.yres - 42, "Camera: (%.0f, %.0f, %.0f)", world->camera.x, world->camera.y, world->camera.z);

			// Show the XYZ coordinates relative to the zero point
			f16->print(5, video.yres - 22, "XYZ: (%.0f, %.0f, %.0f)", -(world->camera.z - ZEROPOINT), -(world->camera.x - ZEROPOINT), (world->camera.y));

            f24->shprint(video.xres - 170, 30, "Controls");
            f16->shprint(video.xres - 240, 66, "F1 - toggle models\n"
                                  "F2 - toggle doodads\n"
                                  "F3 - toggle terrain\n"
                                  "F4 - toggle UI\n"
                                  "F5 - save bookmark\n"
                                  "F6 - toggle map objects\n"
                                  "H - disable highres terrain\n"
                                  "I - toggle invert mouse\n"
                                  "M - minimap\n"
                                  "Esc - back/exit\n"
                                  "WASD - move\n"
                                  "R - quick 180 degree turn\n"
                                  "F - toggle fog\n"
                                  "+,- - adjust fog distance\n"
                                  "O,P - slower/faster movement\n"
                                  "B,N - slower/faster time\n"
            );
		}

		if (world->loading) {
			const char* loadstr = "Loading...";
			const char* oobstr = "Out of bounds";

			f16->print(video.xres/2 - f16->textwidth(loadstr)/2, /*video.yres/2-8*/ 0, world->oob?oobstr:loadstr);
		}

		/*
		f16->print(5,20,"C: %.1f", world->l_const);
		f16->print(5,40,"L: %.2f", world->l_linear);
		f16->print(5,60,"Q: %.3f", world->l_quadratic);
		*/
	}

};

void Test::moveToNearestNode()
{
	if (world->botNodes.nodes.empty())
		return;

	// Find closest node
	float minDist = FLT_MAX;
	const TravelNode* nearestNode = nullptr;

	for (const auto& node : world->botNodes.nodes)
	{
		float dist = (Vec3D(node.x, node.y, node.z) - world->camera).length();
		if (dist < minDist)
		{
			minDist = dist;
			nearestNode = &node;
		}
	}

	if (nearestNode)
	{
		moveToNode(*nearestNode);
		gLog("Moved to nearest node: %s\n", nearestNode->name.c_str());
	}
}

void Test::moveToNode(const TravelNode& node)
{
	// Move camera to slightly above node position
	world->camera = Vec3D(-(node.y - ZEROPOINT), (node.z), -(node.x - ZEROPOINT));

	// Update look target to look forward
	Vec3D lookDir = Vec3D(cosf(10.0f * PI / 180.0f), 10.0f, sinf(10.0f * PI / 180.0f));
	world->lookat = world->camera + lookDir;

	tick(0, 0.001f);

	gLog("Moved to node: %s\n", node.name.c_str());
}

void Test::keypressed(SDL_KeyboardEvent *e)
{
	if (e->type == SDL_KEYDOWN) {
		// key DOWN

		// quit
		if (e->keysym.sym == SDLK_ESCAPE) {
		    gPop = true;
		}

		if (e->keysym.sym == SDLK_F11)
		{
			world->camera = Vec3D(-8949.95, -132.493, 83.5312);
			tick(0, 0.001f);
		}

		if (e->keysym.sym == SDLK_F10)
		{
			world->camera = Vec3D(-8949.95, -132.493, 83.5312);
			tick(0, 0.001f);
		}

		// Go to nearest node
		if (e->keysym.sym == SDLK_n && (e->keysym.mod & KMOD_CTRL)){
			moveToNearestNode();
		}

		// Cycle through nodes
		if (e->keysym.sym == SDLK_g) {
			static size_t currentNode = 0;
			if (!world->botNodes.nodes.empty()) {
				moveToNode(world->botNodes.nodes[currentNode]);
				currentNode = (currentNode + 1) % world->botNodes.nodes.size();
			}
		}

		// movement
		if (e->keysym.sym == SDLK_w) {
			moving = 1.0f;
		}
		if (e->keysym.sym == SDLK_s) {
			moving = -1.0f;
		}
		if (e->keysym.sym == SDLK_a) {
			strafing = -1.0f;
		}
		if (e->keysym.sym == SDLK_d) {
			strafing = 1.0f;
		}
		if (e->keysym.sym == SDLK_e) {
			updown = -1.0f;
		}
		if (e->keysym.sym == SDLK_q) {
			updown = 1.0f;
		}

		// invertmouse
		if (e->keysym.sym == SDLK_i) {
			mousedir *= -1.0f;
		}
		// move speed
		if (e->keysym.sym == SDLK_p) {
			movespd *= 2.0f;
		}
		if (e->keysym.sym == SDLK_o) {
			movespd *= 0.5f;
		}
        if (e->keysym.sym == SDLK_LSHIFT) {
            movespd *= 5.0f;
        }
		// turn around
		if (e->keysym.sym == SDLK_r) {
			ah += 180.0f;
		}

		// testing
		if (e->keysym.sym == SDLK_n) {
			world->modelmanager.v++;
		}

		if (e->keysym.sym == SDLK_b) {
			world->modelmanager.v--;
			if (world->modelmanager.v<0) world->modelmanager.v = 0;
		}

		// toggles
		if (e->keysym.sym == SDLK_t) {
			world->thirdperson = !world->thirdperson;
		}
		if (e->keysym.sym == SDLK_l) {
			world->lighting = !world->lighting;
		}

		if (e->keysym.sym == SDLK_F1) {
			world->drawmodels = !world->drawmodels;
		}
		if (e->keysym.sym == SDLK_F2) {
			world->drawdoodads = !world->drawdoodads;
		}
		if (e->keysym.sym == SDLK_F3) {
			world->drawterrain = !world->drawterrain;
		}
		if (e->keysym.sym == SDLK_F4) {
			hud = !hud;
		}
		if (e->keysym.sym == SDLK_F6) {
			world->drawwmo = !world->drawwmo;
		}
        if (e->keysym.sym == SDLK_F7) {
            world->useshaders = !world->useshaders;
        }
        if (e->keysym.sym == SDLK_F8) {
            reloadShaders();
        }
		if (e->keysym.sym == SDLK_h) {
			world->drawhighres = !world->drawhighres;
		}
		if (e->keysym.sym == SDLK_f) {
			world->drawfog = !world->drawfog;
		}

		if (e->keysym.sym == SDLK_KP_PLUS || e->keysym.sym == SDLK_PLUS) {
			world->fogdistance += 60.0f;
		}
		if (e->keysym.sym == SDLK_KP_MINUS || e->keysym.sym == SDLK_MINUS) {
			world->fogdistance -= 60.0f;
		}

		// minimap
		if (e->keysym.sym == SDLK_m) {
			mapmode = !mapmode;
		}

		/*
		// lighting
		if (e->keysym.sym == SDLK_1) {
			world->l_const -= 0.1f;
			if (world->l_const <= 0) world->l_const = 0.0f;
		}
		if (e->keysym.sym == SDLK_2) {
			world->l_const += 0.1f;
		}
		if (e->keysym.sym == SDLK_3) {
			world->l_linear -= 0.01f;
			if (world->l_linear <= 0) world->l_linear = 0.0f;
		}
		if (e->keysym.sym == SDLK_4) {
			world->l_linear += 0.01f;
		}
		if (e->keysym.sym == SDLK_5) {
			world->l_quadratic -= 0.001f;
			if (world->l_quadratic <= 0) world->l_quadratic = 0.0f;
		}
		if (e->keysym.sym == SDLK_6) {
			world->l_quadratic += 0.001f;
		}
		*/

		if (e->keysym.sym == SDLK_F5) {
			FILE *bf = fopen("bookmarks.txt","a");
			// copied from above: retreive area name for bookmarks, too
			unsigned int areaID = world->getAreaID();
			unsigned int regionID = 0;
			std::string areaName = "";
			try {
				AreaDB::Record rec = gAreaDB.getByAreaID(areaID);
				areaName = rec.getString(AreaDB::Name);
				//regionID = rec.getUInt(AreaDB::Region);
			} catch(AreaDB::NotFound)
			{
				if (world->gnWMO==0) areaName = "Unknown location";
			}
			if (regionID != 0) {
				/// Look up region
				try {
					AreaDB::Record rec = gAreaDB.getByAreaID(regionID);
					areaName = rec.getString(AreaDB::Name);
				} catch(AreaDB::NotFound)
				{
					// do nothing
				}
			}

			fprintf(bf, "%s %f %f %f  %f %f  %s\n", world->basename.c_str(), world->camera.x, world->camera.y, world->camera.z, ah, av, areaName.c_str());
			fclose(bf);
		}
	} else {
		// key UP

		if (e->keysym.sym == SDLK_w) {
			if (moving > 0) moving = 0;
		}
		if (e->keysym.sym == SDLK_s) {
			if (moving < 0) moving = 0;
		}
		if (e->keysym.sym == SDLK_d) {
			if (strafing > 0) strafing = 0;
		}
		if (e->keysym.sym == SDLK_a) {
			if (strafing < 0) strafing = 0;
		}
		if (e->keysym.sym == SDLK_q) {
			if (updown > 0) updown = 0;
		}
		if (e->keysym.sym == SDLK_e) {
			if (updown < 0) updown = 0;
		}
        if (e->keysym.sym == SDLK_LSHIFT) {
            movespd /= 5.0f;
        }
	}
}

void Test::mousemove(SDL_MouseMotionEvent *e)
{
	if (look || fullscreen) {
		ah += e->xrel / XSENS;
		av += mousedir * e->yrel / YSENS;
		if (av < -80) av = -80;
		else if (av > 80) av = 80;
	}

}

void Test::mouseclick(SDL_MouseButtonEvent *e)
{
	if (e->type == SDL_MOUSEBUTTONDOWN) {
		look = true;
	} else if (e->type == SDL_MOUSEBUTTONUP) {
		look = false;
	}

}

