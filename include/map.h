#pragma once
#include <vector>
#include <string>

struct TileCoords {
    int x, y, zoom;
};

struct TileCoordsFloat {
    double x, y;
};

struct GeoCoords {
    double lat, lon;
};

struct MercatorCoords {
    double x, y;
};

class OsmUtils {
public:
    static TileCoords geoToTile(double lat, double lon, int zoom);
    static TileCoordsFloat geoToTileFloat(double lat, double lon, int zoom);
    static std::string getTilePath(const TileCoords& tile);
    static std::string getTileUrl(const TileCoords& tile);
    static MercatorCoords geoToMercator(double lat, double lon);
    static TileCoordsFloat mercatorToTile(MercatorCoords m, int zoom);
};