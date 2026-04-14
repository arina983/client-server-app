#include "TileManager.h"
#include <curl/curl.h>
#include "stb_image.h"
#include <filesystem>
#include <fstream>
#include <iostream>

size_t onPullResponse(void* data, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto& blob = *static_cast<std::vector<unsigned char>*>(userp);
    auto const* const dataptr = static_cast<unsigned char*>(data);
    blob.insert(blob.cend(), dataptr, dataptr + realsize);
    return realsize;
}

GLuint TileManager::getTileTexture(const TileCoords& tile) {
    std::string path = OsmUtils::getTilePath(tile);
    if (texture_cache.count(path)) {
        return texture_cache[path].id;
    }

    GLuint id = downloadAndCreateTexture(tile);
    if (id != 0) {
        texture_cache[path] = {id};
    }
    return id;
}

GLuint TileManager::downloadAndCreateTexture(const TileCoords& tile) {
    std::string path = OsmUtils::getTilePath(tile);
    std::vector<unsigned char> blob;

    if (std::filesystem::exists(path)) {
        std::ifstream file(path, std::ios::binary);
        blob = std::vector<unsigned char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    } else {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string url = OsmUtils::getTileUrl(tile);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &blob);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onPullResponse);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);

            if (curl_easy_perform(curl) == CURLE_OK) {
                std::filesystem::create_directories(std::filesystem::path(path).parent_path());
                std::ofstream out(path, std::ios::binary);
                out.write((char*)blob.data(), blob.size());
            }
            curl_easy_cleanup(curl);
        }
    }

    if (blob.empty()) return 0;

    int w, h, ch;
    unsigned char* data = stbi_load_from_memory(blob.data(), blob.size(), &w, &h, &ch, 4);
    if (!data) return 0;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}

TileManager::~TileManager() {
    for (auto& [path, tex] : texture_cache) {
        glDeleteTextures(1, &tex.id);
    }
}