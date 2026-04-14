#include "map.h"
#include <cmath>
#include <sstream>
#include <filesystem>

TileCoords OsmUtils::geoToTile(double lat, double lon, int zoom) {
    double n = std::pow(2.0, zoom);
    int x = static_cast<int>((lon + 180.0) / 360.0 * n);
    double lat_rad = lat * M_PI / 180.0;
    int y = static_cast<int>((1.0 - std::log(std::tan(lat_rad) + (1.0 / std::cos(lat_rad))) / M_PI) / 2.0 * n);
    return {x, y, zoom};
}
TileCoordsFloat OsmUtils::geoToTileFloat(double lat, double lon, int zoom) {
    double n = std::pow(2.0, zoom);
    double x = (lon + 180.0) / 360.0 * n;
    double lat_rad = lat * M_PI / 180.0;
    double y = (1.0 - std::log(std::tan(lat_rad) + (1.0 / std::cos(lat_rad))) / M_PI) / 2.0 * n;
    return {x, y};
}
std::string OsmUtils::getTilePath(const TileCoords& tile) {
    std::ostringstream ss;
    ss << "zoom/" << tile.zoom << "/" << tile.x << "/" << tile.y << ".png";
    return ss.str();
}

std::string OsmUtils::getTileUrl(const TileCoords& tile) {
    std::ostringstream ss;
    ss << "https://a.tile.openstreetmap.org/" << tile.zoom << "/" << tile.x << "/" << tile.y << ".png";
    return ss.str();
}