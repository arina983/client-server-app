#pragma once
#include "map.h"
#include <GL/glew.h>
#include <map>
#include <string>
#include <vector>

struct TileTexture {
    GLuint id = 0;
};

class TileManager {
public:
    GLuint getTileTexture(const TileCoords& tile);
    ~TileManager();

private:
    std::map<std::string, TileTexture> texture_cache;
    GLuint downloadAndCreateTexture(const TileCoords& tile);
};