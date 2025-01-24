#ifdef _WIN32
#pragma comment(lib,"OpenGL32.lib")
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"SDL.lib")
#pragma comment(lib,"SDLmain.lib")

//#pragma comment(lib,"SFmpq.lib")

#define NOMINMAX
#include <windows.h>
#include <winerror.h>
#endif

#include "SDL.h"
#include "wowmapview.h"

#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

#include "mpq.h"
#include "video.h"
#include "appstate.h"

#include "test.h"
#include "menu.h"
#include "areadb.h"

#include "Database\Database.h"

Database GameDb;

int fullscreen = 0;

std::string gamePath = "D:\\twmoa_1171";//"./";
int expansion = 0;
FILE *flog;
bool glogfirst = true;

std::vector<AppState*> gStates;
bool gPop = false;

float gFPS;

GLuint ftex;
Font *f16, *f24, *f32;

AreaDB gAreaDB;

void initFonts()
{
    ftex = loadTGA("arial.tga", false);
    if (ftex == 0) {
        gLog("Failed to load arial.tga font!\n");
        exit(1);
    }

    f16 = new Font(ftex, 256, 256, 16, "arial.info");
    f24 = new Font(ftex, 256, 256, 24, "arial.info");
    f32 = new Font(ftex, 256, 256, 32, "arial.info");
}

void deleteFonts()
{
    glDeleteTextures(1, &ftex);

    delete f16;
    delete f24;
    delete f32;
}

/*#ifdef _WINDOWS
// HACK: my stupid compiler wont use main()
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    char *argv[] = { "wowmapview.exe", "-w" };
    return main(2,argv);
}
#endif*/

std::string MakeConnectionString()
{
    std::string mysql_host;
    std::string mysql_port;
    std::string mysql_user;
    std::string mysql_pass;
    std::string mysql_db;

    if (mysql_host.empty())
        mysql_host = "127.0.0.1";

    if (mysql_port.empty())
        mysql_port = "3306";

    if (mysql_user.empty())
        mysql_user = "root";

    if (mysql_pass.empty())
        mysql_pass = "root";

    if (mysql_db.empty())
        mysql_db = "vmangos_world_worldbot";

    return mysql_host + ";" + mysql_port + ";" + mysql_user + ";" + mysql_pass + ";" + mysql_db;
}

int main(int argc, char *argv[])
{
    std::string const connection_string = MakeConnectionString();

    printf("\nConnecting to database.\n");
    if (!GameDb.Initialize(connection_string.c_str()))
    {
        printf("\nError: Cannot connect to world database!\n");
        getchar();
        return 1;
    }

    srand((unsigned int)time(0));

    int xres = 1600;
    int yres = 1200;

    bool usePatch = true;

    const char *override_game_path = NULL;
    int maxFps = 60;

    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i],"-gamepath")) {
            i++;
            override_game_path = argv[i];
        }
        else if (!strcmp(argv[i],"-f")) fullscreen = 1;
        else if (!strcmp(argv[i],"-w")) fullscreen = 0;
        else if (!strcmp(argv[i],"-1024") || !strcmp(argv[i],"-1024x768")) {
            xres = 1024;
            yres = 768;
        }
        else if (!strcmp(argv[i],"-1280") || !strcmp(argv[i],"-1280x1024")) {
            xres = 1280;
            yres = 1024;
        }
        else if (!strcmp(argv[i],"-1280x960")) {
            xres = 1280;
            yres = 960;
        }
        else if (!strcmp(argv[i],"-1400") || !strcmp(argv[i],"-1400x1050")) {
            xres = 1400;
            yres = 1050;
        }
        else if (!strcmp(argv[i],"-1280x800")) {
            xres = 1280;
            yres = 800;
        }
        else if (!strcmp(argv[i],"-1600") || !strcmp(argv[i],"-1600x1200")) {
            xres = 1600;
            yres = 1200;
        }
        else if (!strcmp(argv[i],"-1920") || !strcmp(argv[i],"-1920x1200")) {
            xres = 1920;
            yres = 1200;
        }
        else if (!strcmp(argv[i],"-2048") || !strcmp(argv[i],"-2048x1536")) {
            xres = 2048;
            yres = 1536;
        }
        else if (!strcmp(argv[i],"-p")) usePatch = true;
        else if (!strcmp(argv[i],"-np")) usePatch = false;
        else if (!strcmp(argv[i],"-tbc")) expansion = 1;
        else if (!strcmp(argv[i],"-wotlk")) expansion = 2;
        else if (!strcmp(argv[i],"-fps"))
        {
            i++;
            maxFps = std::max(1, atoi(argv[i]));
        }
    }


    if (override_game_path || gamePath != "./")
    {
        if (override_game_path)
        {
            gamePath.clear();
            gamePath.append(override_game_path);
        }

        if (expansion > 0)
            gamePath.append("\\data\\");
        else
            gamePath.append("\\Data\\");
    }
    else
        getGamePath();

    gLog(APP_TITLE " " APP_VERSION "\nGame path: %s\n", gamePath.c_str());


    std::vector<MPQArchive*> archives;
    char path[512];
    // TBC+ have archives in locale folders
    if (expansion == 1)
    {
        // detect locale
        std::string langName;
        const char* locales[] = {"enUS", "enGB", "deDE", "frFR", "zhTW", "ruRU", "esES", "koKR", "zhCN", "ptBR"};
        for (auto & locale : locales)
        {
            sprintf(path, "%s%s\\locale-%s.MPQ", gamePath.c_str(), locale, locale);
            if (file_exists(path)) {
                langName = locale;
                break;
            }
        }
        gLog("Locale: %s\n", langName.c_str());

        // load main archives
        const char* archiveNames[] = {"common.MPQ", "expansion.MPQ"};
        for (auto & archiveName : archiveNames)
        {
            sprintf(path, "%s%s", gamePath.c_str(), archiveName);
            archives.push_back(new MPQArchive(path));
        }

        // load locale archives
        sprintf(path, "%s%s", gamePath.c_str(), std::string(langName + "\\locale-" + langName + ".MPQ").c_str());
        archives.push_back(new MPQArchive(path));
        sprintf(path, "%s%s", gamePath.c_str(), std::string(langName + "\\expansion-locale-" + langName + ".MPQ").c_str());
        archives.push_back(new MPQArchive(path));

        // load patches
        if (usePatch)
        {
            sprintf(path, "%s%s", gamePath.c_str(), "patch.MPQ");
            archives.push_back(new MPQArchive(path));

            {
                auto dirIter = std::filesystem::directory_iterator(gamePath.c_str());
                for (auto& fl : dirIter)
                {
                    if (fl.is_regular_file())
                    {
                        const std::filesystem::path &p(fl.path());
                        if (p.extension().string() != ".mpq" && p.extension().string() != ".MPQ")
                            continue;
                        auto fileName = p.stem().string();
                        if (fileName.find("patch-") == std::string::npos)
                            continue;

                        archives.push_back(new MPQArchive(p.string().c_str()));
                    }
                }
            }

            auto pathLocale = gamePath;
            pathLocale.append(langName + "\\");
            if (std::filesystem::exists(pathLocale))
            {
                sprintf(path, "%s%s", pathLocale.c_str(), std::string("patch-" + langName + ".MPQ").c_str());
                archives.push_back(new MPQArchive(path));
                auto dirIter = std::filesystem::directory_iterator(pathLocale.c_str());
                for (auto& fl : dirIter)
                {
                    if (fl.is_regular_file())
                    {
                        const std::filesystem::path &p(fl.path());
                        if (p.extension().string() != ".mpq" && p.extension().string() != ".MPQ")
                            continue;
                        auto fileName = p.stem().string();
                        if (fileName.find(std::string("patch-" + langName + "-")) == std::string::npos)
                            continue;

                        archives.push_back(new MPQArchive(p.string().c_str()));
                    }
                }
            }
        }
    }
    else if (expansion == 2)
    {
        // detect locale
        std::string langName;
        const char* locales[] = {"enUS", "enGB", "deDE", "frFR", "zhTW", "ruRU", "esES", "koKR", "zhCN", "ptBR"};
        for (auto & locale : locales)
        {
            sprintf(path, "%s%s\\locale-%s.MPQ", gamePath.c_str(), locale, locale);
            //gLog(path);
            if (file_exists(path)) {
                langName = locale;
                break;
            }
        }
        gLog("Locale: %s\n", langName.c_str());

        // load main archives
        const char* archiveNames[] = {"common.MPQ", "common-2.MPQ", "expansion.MPQ", "lichking.MPQ"};
        for (auto & archiveName : archiveNames)
        {
            sprintf(path, "%s%s", gamePath.c_str(), archiveName);
            archives.push_back(new MPQArchive(path));
        }

        // load locale archives
        sprintf(path, "%s%s", gamePath.c_str(), std::string(langName + "\\locale-" + langName + ".MPQ").c_str());
        archives.push_back(new MPQArchive(path));
        sprintf(path, "%s%s", gamePath.c_str(), std::string(langName + "\\expansion-locale-" + langName + ".MPQ").c_str());
        archives.push_back(new MPQArchive(path));
        sprintf(path, "%s%s", gamePath.c_str(), std::string(langName + "\\lichking-locale-" + langName + ".MPQ").c_str());
        archives.push_back(new MPQArchive(path));

        // load patches
        if (usePatch)
        {
            sprintf(path, "%s%s", gamePath.c_str(), "patch.MPQ");
            archives.push_back(new MPQArchive(path));

            {
                auto dirIter = std::filesystem::directory_iterator(gamePath.c_str());
                for (auto& fl : dirIter)
                {
                    if (fl.is_regular_file())
                    {
                        const std::filesystem::path &p(fl.path());
                        if (p.extension().string() != ".mpq" && p.extension().string() != ".MPQ")
                            continue;
                        auto fileName = p.stem().string();
                        if (fileName.find("patch-") == std::string::npos)
                            continue;

                        archives.push_back(new MPQArchive(p.string().c_str()));
                    }
                }
            }

            auto pathLocale = gamePath;
            pathLocale.append(langName + "\\");
            if (std::filesystem::exists(pathLocale))
            {
                sprintf(path, "%s%s", pathLocale.c_str(), std::string("patch-" + langName + ".MPQ").c_str());
                archives.push_back(new MPQArchive(path));
                auto dirIter = std::filesystem::directory_iterator(pathLocale.c_str());
                for (auto& fl : dirIter)
                {
                    if (fl.is_regular_file())
                    {
                        const std::filesystem::path &p(fl.path());
                        if (p.extension().string() != ".mpq" && p.extension().string() != ".MPQ")
                            continue;
                        auto fileName = p.stem().string();
                        if (fileName.find(std::string("patch-" + langName + "-")) == std::string::npos)
                            continue;

                        archives.push_back(new MPQArchive(p.string().c_str()));
                    }
                }
            }
        }
    }
    else
    {
        const char* archiveNames[] = {"texture.MPQ", "model.MPQ", "wmo.MPQ", "terrain.MPQ", "interface.MPQ", "misc.MPQ", "dbc.MPQ"};

        // load main archives
        for (auto & archiveName : archiveNames)
        {
            sprintf(path, "%s%s", gamePath.c_str(), archiveName);
            archives.push_back(new MPQArchive(path));
        }

        // load patches
        if (usePatch)
        {
            auto dirIter = std::filesystem::directory_iterator(gamePath.c_str());
            sprintf(path, "%s%s", gamePath.c_str(), "patch.MPQ");
            archives.push_back(new MPQArchive(path));
            for (auto& fl : dirIter)
            {
                if (fl.is_regular_file())
                {
                    const std::filesystem::path &p(fl.path());
                    if (p.extension().string() != ".mpq" && p.extension().string() != ".MPQ")
                        continue;
                    auto fileName = p.stem().string();
                    if (fileName.find("patch-") == std::string::npos)
                        continue;

                    archives.push_back(new MPQArchive(p.string().c_str()));
                }
            }
        }
    }

    gLog("Opening Area DBC Files...\n");
    gAreaDB.open();

    video.init(xres,yres,fullscreen!=0);
    SDL_WM_SetCaption(APP_TITLE,NULL);

    if (!(supportVBO && supportMultiTex)) {
        video.close();
        const char *msg = "Error: Cannot initialize OpenGL extensions.";
        gLog("%s\n",msg);
        gLog("Missing required extensions:\n");
        if (!supportVBO) gLog("GL_ARB_vertex_buffer_object\n");
        if (!supportMultiTex) gLog("GL_ARB_multitexture\n");
#ifdef _WIN32
        MessageBox(0, msg, 0, MB_OK|MB_ICONEXCLAMATION);
        exit(1);
#endif
    }

    initFonts();


    float ftime;
    Uint32 t, last_t = 0, frames = 0, time = 0, fcount = 0, ft = 0;
    AppState *as;
    gFPS = 0;
    double delta = 0;
    unsigned int a;
    unsigned int b;

    Menu *m = new Menu();
    as = m;

    gStates.push_back(as);
    if (gStates.size() > 100)
        gStates.erase(gStates.begin(), gStates.begin() + 99);
    //gStates.erase(gStates.begin());

    bool done = false;
    t = SDL_GetTicks();
    while(!gStates.empty() && !done) {
        t = SDL_GetTicks();
        Uint32 dt = t - last_t;
        time += dt;
        ftime = time / 1000.0f;
        if (last_t && dt && dt < Uint32(1000/maxFps))
            continue;

        last_t = t;

        as = gStates[gStates.size()-1];

        SDL_Event event;
        while ( SDL_PollEvent(&event) ) {
            if ( event.type == SDL_QUIT ) {
                done = true;
            }
            else if ( event.type == SDL_MOUSEMOTION) {
                as->mousemove(&event.motion);
            }
            else if ( event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                as->mouseclick(&event.button);
            }
            else if ( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                as->keypressed(&event.key);
            }
        }

        as->tick(ftime, dt/1000.0f);

        as->display(ftime, dt/1000.0f);

        if (gPop) {
            gPop = false;
            gStates.pop_back();
            delete as;
        }

        frames++;
        fcount++;
        ft += dt;
        if (ft >= 1000) {
            float fps = (float)fcount / (float)ft * 1000.0f;
            gFPS = fps;
            char buf[32];
            sprintf(buf, APP_TITLE " - %.2f fps",fps);
            SDL_WM_SetCaption(buf,NULL);
            ft = 0;
            fcount = 0;
        }

        video.flip();

    }


    deleteFonts();

    video.close();

    for (auto it = archives.begin(); it != archives.end(); ++it) {
        (*it)->close();
    }
    archives.clear();

    gLog("\nExiting.\n");

    return 0;
}

float frand()
{
    return rand()/(float)RAND_MAX;
}

float randfloat(float lower, float upper)
{
    return lower + (upper-lower)*(rand()/(float)RAND_MAX);
}

int randint(int lower, int upper)
{
    return lower + (int)((upper+1-lower)*frand());
}

void fixnamen(char *name, size_t len)
{
    for (size_t i=0; i<len; i++) {
        if (i>0 && name[i]>='A' && name[i]<='Z' && isalpha(name[i-1])) {
            name[i] |= 0x20;
        } else if ((i==0 || !isalpha(name[i-1])) && name[i]>='a' && name[i]<='z') {
            name[i] &= ~0x20;
        }
    }
}

void fixname(std::string &name)
{
    for (size_t i=0; i<name.length(); i++) {
        if (i>0 && name[i]>='A' && name[i]<='Z' && isalpha(name[i-1])) {
            name[i] |= 0x20;
        } else if ((i==0 || !isalpha(name[i-1])) && name[i]>='a' && name[i]<='z') {
            name[i] &= ~0x20;
        }
    }
}

int file_exists(const char *path)
{
#ifdef _WINDOWS
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
#else
    if (access(path,R_OK) == 0) return true;
#endif
    return false;
};

// String Effectors
void Lower(std::string &text){
    transform(text.begin(), text.end(), text.begin(), (int(*)(int)) std::tolower);
};
bool StartsWith (std::string const &fullString, std::string const &starting)
{
    if (fullString.length() > starting.length()) {
        return (0 == fullString.compare(0, starting.length(), starting));
    } else {
        return false;
    }
};

bool EndsWith (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() > ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
};

std::string BeforeLast(const char* String, const char* Last)
{
    std::string a = String;
    size_t ending = a.rfind(Last);
    if (!ending)
        ending = a.length();
    std::string finalString = a.substr(0,ending);

    return finalString;
};

std::string AfterLast(const char* String, const char* Last)
{
    std::string a = String;
    size_t ending = a.rfind(Last);
    if (!ending)
        ending = 0;
    std::string finalString = a.substr(ending,(a.length()-ending));

    return finalString;
};

std::string GetLast(const char* String){
    std::string a = String;
    size_t b = a.length();
    return a.substr(b-1,b);
};

void getGamePath()
{
#ifdef _WIN32
    HKEY key;
    unsigned long t, s;
    long l;
    unsigned char path[1024];
    //memset(path, 0, sizeof(path));
    s = 1024;
    memset(path,0,s);

    l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Blizzard Entertainment\\World of Warcraft",0,KEY_QUERY_VALUE,&key);
    if (l == ERROR_SUCCESS) {
        l = RegQueryValueEx(key,"InstallPath",0,&t,(LPBYTE)path,&s);
        RegCloseKey(key);
        gamePath.clear();
        gamePath.append(reinterpret_cast<const char*>(path));
        gamePath.append("Data\\");
    }
#else
    strcpy(gamepath,"data/");
#endif
}

void gLog(const char *str, ...)
{
    if (glogfirst) {
        flog = fopen("log.txt","w");
        fclose(flog);
        glogfirst = false;
    }

    flog = fopen("log.txt","a");

    va_list ap;

            va_start (ap, str);
    vfprintf (flog, str, ap);
            va_end (ap);

            va_start (ap,str);
    vprintf(str,ap);
            va_end(ap);

    fclose(flog);
};
