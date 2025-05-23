// Mugen Sprite (SFF) Viewer
// by leonkasovan@gmail.com, (c) 27 April 2025

#define STB_RECT_PACK_IMPLEMENTATION
#include "mugen_sff.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "glad.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <filesystem>
#include <cstring>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <limits.h>
#endif


#define Window_w 640
#define Window_h 480

// Global variable
GLuint g_shaderProgram, g_RGBAShaderProgram, g_PalettedShaderProgram;
GLuint g_quadVAO, g_quadVBO;
GLint g_texLocation, g_paletteLocation, g_positionLocation, g_sizeLocation, g_windowSizeLocation;

std::map<int, std::string> compression_format_code = {
    {-1, "PCX"},
    {-2, "RLE8"},
    {-3, "RLE5"},
    {-4, "LZ5"},
    {-10, "PNG10"},
    {-11, "PNG11"},
    {-12, "PNG12"}
};

const char* global_vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform vec2 uPosition;
uniform vec2 uSize;
uniform vec2 uWindowSize;

out vec2 TexCoord;

void main() {
    vec2 scaledPos = aPos * uSize;
    vec2 worldPos = uPosition + scaledPos;
    vec2 ndcPos = vec2((worldPos.x / uWindowSize.x) * 2.0 - 1.0,
                       1.0 - (worldPos.y / uWindowSize.y) * 2.0);
    gl_Position = vec4(ndcPos, 0.0, 1.0);
    TexCoord = aTexCoord;
})";

// Shader for The Paletted texture
const char* Paletted_fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D tex;
uniform sampler2D paletteTex;
void main() {
    FragColor = texture(paletteTex, vec2(texture(tex, TexCoord).r, 0.5));
})";

// Shader for The RGBA texture
const char* RGBA_fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord; // Received from vertex shader
out vec4 FragColor; 
uniform sampler2D tex; 
void main() {
    FragColor = texture(tex, TexCoord);
})";

std::string getExecutableDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH); // A version = narrow char
    std::filesystem::path exe_path(path);
    return exe_path.parent_path().string();
#else
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count == -1) return ".";
    std::filesystem::path exe_path(std::string(path, count));
    return exe_path.parent_path().string();
#endif
}

// Get the base filename without extension
std::string getFilenameNoExt(const char* fullpath) {
    if (!fullpath) return "";

    std::string path(fullpath);

    // Find last path separator (both Unix '/' and Windows '\\')
    size_t slashPos = path.find_last_of("/\\");
    size_t start = (slashPos == std::string::npos) ? 0 : slashPos + 1;

    // Find last dot after the last slash
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || dotPos < start) {
        dotPos = path.length();  // No extension found
    }

    return path.substr(start, dotPos - start);
}

#ifdef _WIN32
std::string GetExecutablePath() {
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        // std::wcerr << "Failed to get executable path.\n";
        fprintf(stderr, "Failed to get executable path.\n");
        return "";
    }
    return std::string(buffer);
}

// Helper to write a registry value
bool WriteRegKey(HKEY root, const std::string& subkey, const std::string& valueName, const std::string& valueData) {
    HKEY hKey;
    LONG result = RegCreateKeyEx(root, subkey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) {
        // std::cerr << "Failed to create/open key: " << subkey << " (Error " << result << ")\n";
        fprintf(stderr, "Failed to create/open key: %s (Error %ld)\n", subkey.c_str(), result);
        return false;
    }

    result = RegSetValueEx(hKey, valueName.c_str(), 0, REG_SZ,
        (const BYTE*) valueData.c_str(),
        static_cast<DWORD>((valueData.size() + 1) * sizeof(wchar_t)));

    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

// Helper to delete a registry key and all subkeys
bool DeleteRegKey(HKEY root, const std::string& subkey) {
    LONG result = RegDeleteTree(root, subkey.c_str());
    if (result == ERROR_SUCCESS) {
        // std::cout << "Deleted key: " << subkey << "\n";
        printf("Deleted key: %s\n", subkey.c_str());
        return true;
    } else if (result == ERROR_FILE_NOT_FOUND) {
        // std::cout << "Key not found: " << subkey << "\n";
        printf("Key not found: %s\n", subkey.c_str());
        return true; // Not an error
    } else {
        // std::cerr << "Failed to delete key: " << subkey << " (Error " << result << ")\n";
        fprintf(stderr, "Failed to delete key: %s (Error %ld)\n", subkey.c_str(), result);
        return false;
    }
}

// Helper to delete a value (e.g., file type association)
bool DeleteRegValue(HKEY root, const std::string& subkey, const std::string& valueName) {
    HKEY hKey;
    if (RegOpenKeyEx(root, subkey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        LONG result = RegDeleteValue(hKey, valueName.c_str());
        RegCloseKey(hKey);
        if (result == ERROR_SUCCESS) {
            // std::cout << "Deleted value: " << valueName << " in " << subkey << "\n";
            printf("Deleted value: %s in %s\n", valueName.c_str(), subkey.c_str());
            return true;
        }
    }
    return false;
}

int RegisterSFFHandler() {
    std::string exePath = GetExecutablePath();
    if (exePath.empty()) return -1;

    std::string extension = ".sff";
    std::string fileType = "Mugen.SFF";
    std::string description = "Mugen Sprite File";

    // 1. Associate .sff with file type
    WriteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\" + extension, "", fileType);

    // 2. Describe the file type
    WriteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\" + fileType, "", description);

    // 3. Set default icon
    WriteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\" + fileType + "\\DefaultIcon", "", exePath + ",0");

    // 4. Set open command
    WriteRegKey(HKEY_CURRENT_USER,
        "Software\\Classes\\" + fileType + "\\shell\\open\\command",
        "",
        "\"" + exePath + "\" \"%1\"");

    // 5. Notify Windows of the change
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    // std::cout << ".sff file association registered for " << exePath << "\n";
    return 0;
}

int UnRegisterSFFHandler() {
    std::string extension = ".sff";
    std::string fileType = "Mugen.SFF";

    // 1. Remove file type association
    DeleteRegValue(HKEY_CURRENT_USER, "Software\\Classes\\" + extension, "");

    // 2. Delete file type definition and handlers
    DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\" + fileType);

    // 3. Optionally, delete the extension key itself (only if it's exclusively used)
    DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\" + extension);

    // 4. Notify Windows of the change
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    // std::wcout << L".sff file association unregistered.\n";
    return 0;
}
#endif

inline bool isACT(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    if (ext.length() != 4 || ext[0] != '.') return false;
    return (ext[1] == 'a' || ext[1] == 'A') &&
        (ext[2] == 'c' || ext[2] == 'C') &&
        (ext[3] == 't' || ext[3] == 'T');
}

// Find and Populate files
std::vector<std::string> findACTFiles(const std::filesystem::path& root = std::filesystem::current_path()) {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.is_regular_file() && isACT(entry.path())) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

std::vector<Palette> generateTextureFromPalettes(std::vector<std::string> pals) {
    std::vector<Palette> result;
    result.reserve(pals.size());  // Optional: reserve for efficiency

    for (const auto& path : pals) {
        result.emplace_back(path.c_str());  // Calls Palette(const char* actFilename)
    }

    return result;
}

int exportAllSpriteAsPNG(Sff& sff) {
    char png_filename[256];
    std::string basename = getFilenameNoExt(sff.filename);
    size_t n_success = 0;
    size_t n_failed = 0;

    //Iterate through all sprites and export them as PNG
    for (size_t i = 0; i < sff.header.NumberOfSprites; ++i) {
        Sprite& spr = sff.sprites[i];
        snprintf(png_filename, sizeof(png_filename), "%s %d_%d.png", basename.c_str(), spr.Group, spr.Number);
        printf("Exporting %s\n", png_filename);

        if (isRGBASprite(spr)) { // PNG Image (RGBA)
            exportRGBASpriteAsPng(spr, png_filename) ? ++n_failed : ++n_success;
        } else { // Paletted Image (R only)
            exportPalettedSpriteAsPng(spr, sff.palettes[spr.palidx].texture_id, png_filename) ? ++n_failed : ++n_success;
        }
    }
    printf("Exported %zu sprites successfully, %zu failed. Total=%zu\n", n_success, n_failed, sff.sprites.size());
    return n_success;
}

// int optimizeSpritePalette(Sff& sff) {
//     return 0;
// }

int exportCurrentSpriteAsPNG(Sff& sff, int64_t spr_idx) {
    Sprite& spr = sff.sprites[spr_idx];
    GLuint pal_texture_id = sff.palettes[spr.palidx].texture_id;
    char png_filename[256];
    std::string basename = getFilenameNoExt(sff.filename);
    size_t n_success = 0;
    size_t n_failed = 0;

    snprintf(png_filename, sizeof(png_filename), "%s_%d_%d.png", basename.c_str(), spr.Group, spr.Number);

    if (isRGBASprite(spr)) { // PNG Image (RGBA)
        exportRGBASpriteAsPng(spr, png_filename) ? ++n_failed : ++n_success;
    } else { // Paletted Image (R only)
        exportPalettedSpriteAsPng(spr, pal_texture_id, png_filename) ? ++n_failed : ++n_success;
    }
    return n_success;
}

// Copy raw image data from sprite to a buffer
// This function assumes that the sprite is already bound to a texture
// Don't forget to free the allocated memory after use
unsigned char* copyRawImageFromSprite(Sprite& spr) {
    int width = spr.Size[0];
    int height = spr.Size[1];
    bool isRGBA = isRGBASprite(spr);

    unsigned char* data = (unsigned char*) malloc(width * height * (isRGBA ? 4 : 1));
    if (!data) return NULL;

    glBindTexture(GL_TEXTURE_2D, spr.texture_id);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, isRGBA ? GL_RGBA : GL_RED, GL_UNSIGNED_BYTE, data);

    return data;
}

int getDefaultPaletteIndex(Sff& sff) {
    int default_palette_index = -1;
    for (size_t i = 0; i < sff.header.NumberOfSprites; ++i) {
        if (sff.sprites[i].Group == 0 && sff.sprites[i].Number == 0) {
            // Check if the sprite is a palette sprite
            if (sff.sprites[i].rle == -1 || sff.sprites[i].rle == -2 || sff.sprites[i].rle == -3 || sff.sprites[i].rle == -4 || sff.sprites[i].rle == -10) {
                default_palette_index = sff.sprites[i].palidx;
                break;
            }
        }
    }
    return default_palette_index; // Default palette not found
}

#define META_LINE_LENGTH 32
#define PALETTE_SIZE 256

int exportAllSpriteAsAtlas(Sff& sff) {
    char out_filename[256];
    std::string basename = getFilenameNoExt(sff.filename);
    // size_t n_success = 0;
    // size_t n_failed = 0;

    stbrp_context ctx;
    stbrp_node* nodes = NULL;
    char* meta = NULL;
    uint8_t* output = NULL;
    Atlas atlas;
    int default_palette_index = getDefaultPaletteIndex(sff);

    // Initialize atlas
    int64_t prod = 0;
    size_t maxw = 0, maxh = 0;

    uint32_t num_sprites = sff.header.NumberOfSprites;
    atlas.rects = (struct stbrp_rect*) calloc(num_sprites, sizeof(struct stbrp_rect));
    if (!atlas.rects) return -1;

    for (size_t i = 0; i < num_sprites; i++) {
        Sprite& spr = sff.sprites[i];

        if (isRGBASprite(spr)) {
            continue; // Skip RGBA sprites for now
        }

        if (spr.palidx != default_palette_index) {
            continue; // Skip sprites with different palette index
        }

        unsigned char* p_img = copyRawImageFromSprite(spr);
        int64_t sw = spr.Size[0];
        int64_t sh = spr.Size[1];
        size_t pitch = sw;
        // fprintf(stderr, "Packing spr[%lld] %u,%u %llux%llu (%d,%d) pal=%d rle=%d\n", i, spr.Group, spr.Number, sw, sh, spr.Offset[0], spr.Offset[1], spr.palidx, spr.rle);

        spr.atlas_x = 0;
        spr.atlas_y = 0;

        if (sw > (int64_t) maxw) maxw = sw;
        if (sh > (int64_t) maxh) maxh = sh;
        prod += sw * sh;

        // Crop top
        while (sh > 0) {
            int empty = 1;
            for (int64_t x = 0; x < sw; x++) {
                if (p_img[x + spr.atlas_y * pitch]) {
                    empty = 0;
                    break;
                }
            }
            if (!empty) break;
            spr.atlas_y++;
            sh--;
        }

        // Crop bottom
        while (sh > 0) {
            int empty = 1;
            for (int64_t x = 0; x < sw; x++) {
                if (p_img[x + (spr.atlas_y + sh - 1) * pitch]) {
                    empty = 0;
                    break;
                }
            }
            if (!empty) break;
            sh--;
        }

        // Crop left
        while (sw > 0) {
            int empty = 1;
            for (int64_t y = 0; y < sh; y++) {
                if (p_img[(spr.atlas_y + y) * pitch + spr.atlas_x]) {
                    empty = 0;
                    break;
                }
            }
            if (!empty) break;
            spr.atlas_x++;
            sw--;
        }

        // Crop right
        while (sw > 0) {
            int empty = 1;
            for (int64_t y = 0; y < sh; y++) {
                if (p_img[(spr.atlas_y + y) * pitch + spr.atlas_x + sw - 1]) {
                    empty = 0;
                    break;
                }
            }
            if (!empty) break;
            sw--;
        }

        if (sw < 1 || sh < 1) {
            sw = sh = spr.atlas_x = spr.atlas_y = 0;
        }

        atlas.rects[i].id = i;
        atlas.rects[i].w = sw;
        atlas.rects[i].h = sh;

        if (p_img) free(p_img);
    }

    fprintf(stderr, "Atlas Max width: %zu, Max height: %zu\n", maxw, maxh);
    // Calculate atlas size rounded up to next power of two
    size_t root = 1;
    while (root * root < (size_t) prod) root++;

    if (root < maxw) root = maxw;
    for (atlas.width = 1; atlas.width < root; atlas.width <<= 1);

    size_t rows = (prod + atlas.width - 1) / atlas.width;
    if (rows < maxh) rows = maxh;
    for (atlas.height = 1; atlas.height < rows; atlas.height <<= 1);

    nodes = (stbrp_node*) calloc(atlas.width + 1, sizeof(stbrp_node));
    if (!nodes) {
        fprintf(stderr, "Error: not enough memory for stbrp nodes\n");
        return -1;
    }

    stbrp_init_target(&ctx, atlas.width, atlas.height, nodes, atlas.width + 1);
    if (!stbrp_pack_rects(&ctx, atlas.rects, num_sprites)) {
        atlas.height <<= 1;
        // memset(nodes, 0, (atlas.width + 1) * sizeof(stbrp_node));
        for (uint32_t i = 0; i < num_sprites; i++) {
            atlas.rects[i].was_packed = 0;
            atlas.rects[i].x = atlas.rects[i].y = 0;
        }
        stbrp_init_target(&ctx, atlas.width, atlas.height, nodes, atlas.width + 1);
        if (!stbrp_pack_rects(&ctx, atlas.rects, num_sprites)) {
            fprintf(stderr, "Error: sprites do not fit into %u x %u atlas.\n", atlas.width, atlas.height);
            free(nodes);
            return -2;
        }
    }
    free(nodes);

    uint32_t meta_len = 0, max_x = 0, max_y = 0;
    for (uint32_t i = 0; i < num_sprites; i++) {
        uint32_t right = atlas.rects[i].x + atlas.rects[i].w;
        uint32_t bottom = atlas.rects[i].y + atlas.rects[i].h;
        if (right > max_x) max_x = right;
        if (bottom > max_y) max_y = bottom;
        meta_len += META_LINE_LENGTH + PALETTE_SIZE;
    }

    atlas.width = max_x;
    atlas.height = max_y;
    fprintf(stderr, "Atlas size: %u x %u\n", atlas.width, atlas.height);

    if (atlas.width == 0 || atlas.height == 0) {
        fprintf(stderr, "Error: empty atlas after cropping (%u x %u)\n", atlas.width, atlas.height);
        return -3;
    }

    meta = (char*) calloc(meta_len, 1);
    output = (uint8_t*) calloc(atlas.width * atlas.height, 1);
    if (!meta || !output) {
        fprintf(stderr, "Error: not enough memory for atlas buffers\n");
        free(meta);
        free(output);
        return -4;
    }

    char* meta_ptr = meta;
    for (uint32_t i = 0; i < num_sprites; i++) {
        Sprite& spr = sff.sprites[i];
        if (isRGBASprite(spr))
            continue; // Skip RGBA sprites for now

        if (spr.palidx != default_palette_index) {
            continue; // Skip sprites with different palette index
        }

        unsigned char* raw_image_data = copyRawImageFromSprite(spr);
        char filename[256];
        snprintf(filename, sizeof(filename), "%d_%d", spr.Group, spr.Number);

        if (atlas.rects[i].w > 0 && atlas.rects[i].h > 0) {
            uint8_t* src = raw_image_data + (spr.atlas_y * spr.Size[0] + spr.atlas_x);
            uint8_t* dst = output + (atlas.width * atlas.rects[i].y + atlas.rects[i].x);
            for (int j = 0; j < atlas.rects[i].h; j++) {
                memcpy(dst, src, atlas.rects[i].w);
                dst += atlas.width;
                src += spr.Size[0];
            }
        }
        free(raw_image_data);

#ifdef __MINGW64__
        const char* output_format = "%u\t%u\t%u\t%u\t%llu\t%llu\t%u\t%u\t%s\n";
#else
        const char* output_format = "%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\n";
#endif
        meta_ptr += sprintf(meta_ptr, output_format,
            atlas.rects[i].x, atlas.rects[i].y, atlas.rects[i].w, atlas.rects[i].h,
            spr.atlas_x, spr.atlas_y, spr.Size[0], spr.Size[1], filename);
    }
    free(atlas.rects);

    // Save the atlas metadata to a text file
    snprintf(out_filename, sizeof(out_filename), "sprite_atlas_%s.txt", basename.c_str());
    FILE* f = fopen(out_filename, "w");
    if (f) {
        fwrite(meta, 1, meta_ptr - meta, f);
        fclose(f);
    }
    free(meta);

    // Prepare for saving the atlas as PNG
    snprintf(out_filename, sizeof(out_filename), "sprite_atlas_%s.png", basename.c_str());
    LodePNGState state;
    lodepng_state_init(&state);

    // Set color type to palette
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;

    // Load the palette RGBA data from palette texture (in GPU)
    uint8_t palette_rgba[256 * 4]; // 256 colors, 4 bytes per color (RGBA)
    glBindTexture(GL_TEXTURE_2D, sff.palettes[default_palette_index].texture_id);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);  // Ensure byte-aligned rows
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, palette_rgba);
    for (int i = 0; i < 256; i++) {
        lodepng_palette_add(&state.info_raw, palette_rgba[i * 4 + 0], palette_rgba[i * 4 + 1], palette_rgba[i * 4 + 2], palette_rgba[i * 4 + 3]);
    }
    lodepng_palette_add(&state.info_png.color, 0, 0, 0, 0);	// atleast one color is needed for info_png palette. it will crash if not added

    // Save atlas image output to PNG file
    unsigned char* png = NULL;
    size_t pngsize = 0;
    int err_code = lodepng_encode(&png, &pngsize, output, atlas.width, atlas.height, &state);
    if (!err_code) {
        err_code = lodepng_save_file(png, pngsize, out_filename);
        if (err_code) {
            fprintf(stderr, "Error saving PNG file: %s\n", lodepng_error_text(err_code));
        }
    } else {
        fprintf(stderr, "Error encoding PNG data: %s\n", lodepng_error_text(err_code));
    }
    lodepng_state_cleanup(&state);
    free(png);
    free(output);
    return err_code;
}

int exportSpriteDatabase(Sff& sff) {
    char out_filename[256];
    std::string basename = getFilenameNoExt(sff.filename);
    snprintf(out_filename, sizeof(out_filename), "sprite_database_%s.txt", basename.c_str());
    FILE* f = fopen(out_filename, "w");
    if (!f) {
        fprintf(stderr, "Error opening file for writing: %s\n", out_filename);
        return -1;
    }

    for (size_t i = 0; i < sff.header.NumberOfSprites; ++i) {
        Sprite& spr = sff.sprites[i];
        fprintf(f, "%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\n",
            spr.Group, spr.Number, spr.Size[0], spr.Size[1],
            spr.Offset[0], spr.Offset[1], spr.palidx, -spr.rle,
            compression_format_code[spr.rle].c_str());
    }
    fclose(f);
    return 0;
}

const char* getFilename(const char* fullpath) {
    if (!fullpath) return "";

    const char* slash1 = std::strrchr(fullpath, '/');  // Unix-style
    const char* slash2 = std::strrchr(fullpath, '\\'); // Windows-style

    const char* lastSlash = slash1 > slash2 ? slash1 : slash2;

    return lastSlash ? lastSlash + 1 : fullpath;
}

void SetShader(GLuint program) {
    if (g_shaderProgram != program) {
        glUseProgram(program);
        g_shaderProgram = program;
    }
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Shader Compile Error: %s\n", infoLog);
    }
    return shader;
}

GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void setupQuad() {
    float quadVertices[] = {
        // Positions   // UVs
        0.0f, 1.0f,     0.0f, 1.0f,
        0.0f, 0.0f,     0.0f, 0.0f,
        1.0f, 0.0f,     1.0f, 0.0f,
        1.0f, 1.0f,     1.0f, 1.0f
    };

    glGenVertexArrays(1, &g_quadVAO);
    glGenBuffers(1, &g_quadVBO);
    glBindVertexArray(g_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void renderSprite(Sprite& spr, GLuint paletteTex, float x, float y, float scale = 1.0f) {
    if (isRGBASprite(spr))
        SetShader(g_RGBAShaderProgram);
    else
        SetShader(g_PalettedShaderProgram);
    glBindVertexArray(g_quadVAO);

    // New: scaled size
    float scaledWidth = spr.Size[0] * scale;
    float scaledHeight = spr.Size[1] * scale;

    glUniform2f(g_positionLocation, x, y);
    glUniform2f(g_sizeLocation, scaledWidth, scaledHeight);
    glUniform2f(g_windowSizeLocation, Window_w, Window_h);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, spr.texture_id);
    glUniform1i(g_texLocation, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, paletteTex);
    glUniform1i(g_paletteLocation, 1);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

void showSpriteStatistics(Sff& sff, std::vector<char> text) {
    ImGui::InputTextMultiline("##sprite_statistic", text.data(), text.size(), ImVec2(300, ImGui::GetTextLineHeight() * 16));
    if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
    }
}

// Display sprite statistics:
// 1. Palette usage
// 2. Compression format usage
std::vector<char> getSpriteStatistics(Sff& sff) {
    std::vector<char> res;
    std::ostringstream ss_output;

    ss_output << "Filename: " << getFilename(sff.filename) << "\n";
    ss_output << "SFF Version: " << (int) sff.header.Ver0 << "." << (int) sff.header.Ver1 << "." << (int) sff.header.Ver2 << "." << (int) sff.header.Ver3 << "\n\n";
    ss_output << "Total Sprites: " << sff.header.NumberOfSprites << "\n";
    ss_output << "\tNormal Sprites: " << sff.header.NumberOfSprites - sff.numLinkedSprites << "\n";
    ss_output << "\tLinked Sprites: " << sff.numLinkedSprites << "\n\n";
    ss_output << "Total Palettes: " << sff.header.NumberOfPalettes << "\n\n";

    ss_output << "Compression Usage:\n";
    for (const auto& pair : sff.compression_format_usage) {
        ss_output << "\t" << compression_format_code[pair.first] << ":\t" << pair.second << " sprites\n";
    }
    ss_output << "\nPalette Usage:\n";
    for (const auto& pair : sff.palette_usage) {
        ss_output << "\tPal " << pair.first << ":\t" << pair.second << " sprites\n";
    }
    std::string stringUsage = ss_output.str();
    res.assign(stringUsage.begin(), stringUsage.end());
    res.push_back('\0');
    return res;
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
#ifdef _WIN32
        RegisterSFFHandler();
        MessageBox(NULL, "Usage:\n\t1. MugenSpriteViewer.exe [filename]\n\t2. Just double click SFF file in Windows Explorer", "Mugen Sprite Viewer", MB_OK);
#else
        printf("MugenSpriteViewer\nUsage: %s [filename]\n", argv[0]);
#endif   
        return -1;
    }

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Mugen Sprite Viewer v1.0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Window_w, Window_h, window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Load OpenGL Extention via glad
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    // Setup shader
    g_RGBAShaderProgram = createShaderProgram(global_vertexShaderSource, RGBA_fragmentShaderSource);
    g_PalettedShaderProgram = createShaderProgram(global_vertexShaderSource, Paletted_fragmentShaderSource);
    SetShader(g_PalettedShaderProgram);  // use g_PalettedShaderProgram as default shader

    setupQuad();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_texLocation = glGetUniformLocation(g_shaderProgram, "tex");
    g_paletteLocation = glGetUniformLocation(g_shaderProgram, "paletteTex");
    g_positionLocation = glGetUniformLocation(g_shaderProgram, "uPosition");
    g_sizeLocation = glGetUniformLocation(g_shaderProgram, "uSize");
    g_windowSizeLocation = glGetUniformLocation(g_shaderProgram, "uWindowSize");

    // Sprite Global Variable
    Sff sff;
    static int64_t spr_idx = 0;   // Sprite index to be displayed
    static float spr_zoom = 1.0f;
    static bool spr_auto_animate = false; // Auto animate sprite
    size_t o_palidx = 0;
    std::vector<std::string> opt_palette_paths = findACTFiles(); // Find ACT files from the current directory
    std::vector<Palette> opt_palettes = generateTextureFromPalettes(opt_palette_paths); // Texture palettes from ACT files
    bool useOptPalette = false; // Use optional palette instead of internal palette
    size_t modal_return_status = 0;

    // Generating Sprite's Texture and Palette's Texture from SFF file
    if (loadMugenSprite(argv[1], &sff) != 0) {
        fprintf(stderr, "Failed to load Mugen Sprite %s\n", argv[1]);
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void) io;
    static std::string ini_path = getExecutableDirectory() + "/imgui.ini";
    io.IniFilename = ini_path.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    // bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    int showModal = 0; // 0: no modal, 1: export all, 2: export current
    const char* modalName[] = { "", "Export All Sprite", "Export Current Sprite", "Export as Sprite Atlas", "Export Sprite Database", "View Sprite Statistics", "Register SFF Handler", "UnRegister SFF Handler" };

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_RIGHT:
                case SDLK_PAGEDOWN:
                    spr_idx++;
                    break;
                case SDLK_BACKSPACE:
                case SDLK_PAGEUP:
                case SDLK_LEFT:
                    spr_idx--;
                    break;
                case SDLK_HOME:
                    spr_idx = 0;
                    break;
                case SDLK_ESCAPE:
                case SDLK_q:
                    done = true;
                    break;
                case SDLK_END:
                    spr_idx = sff.header.NumberOfSprites - 1;
                    break;
                case SDLK_SPACE:
                    spr_auto_animate = !spr_auto_animate;
                    break;
                default:
                    break;
                }
            }
        }

        if (spr_auto_animate) {
            // Auto animate sprite
            SDL_Delay(80);
            spr_idx++;
            if (spr_idx >= sff.header.NumberOfSprites) {
                spr_idx = 0;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Mugen Sprite Information");
        ImGui::Text("Filename: %s", getFilename(sff.filename));
        ImGui::Text("Version: %d.%d.%d.%d", sff.header.Ver0, sff.header.Ver1, sff.header.Ver2, sff.header.Ver3);
        ImGui::Text("Total Sprites: %u", sff.header.NumberOfSprites);
        ImGui::Text("Total Palettes: %u", sff.header.NumberOfPalettes);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem(modalName[1])) {
                modal_return_status = exportAllSpriteAsPNG(sff);
                showModal = 1;
            }
            if (ImGui::MenuItem(modalName[2])) {
                modal_return_status = exportCurrentSpriteAsPNG(sff, spr_idx);
                showModal = 2;
            }
            if (ImGui::MenuItem(modalName[3])) {
                modal_return_status = exportAllSpriteAsAtlas(sff);
                showModal = 3;
            }
            if (ImGui::MenuItem(modalName[4])) {
                modal_return_status = exportSpriteDatabase(sff);
                showModal = 4;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(modalName[5])) {
                modal_return_status = 0;
                showModal = 5;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(modalName[6])) {
                modal_return_status = RegisterSFFHandler();
                showModal = 6;
            }
            if (ImGui::MenuItem(modalName[7])) {
                modal_return_status = UnRegisterSFFHandler();
                showModal = 7;
            }
            ImGui::EndPopup();
        }
        ImGui::End();

        if (showModal) {
            ImGui::OpenPopup(modalName[showModal]);
            showModal = 0;
        }

        // Modal for Export All Sprites
        if (ImGui::BeginPopupModal(modalName[1], NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
#ifdef __MINGW64__
            ImGui::Text("%lld of %d sprites exported successfully.", modal_return_status, sff.header.NumberOfSprites);
#else
            ImGui::Text("%ld of %d sprites exported successfully.", modal_return_status, sff.header.NumberOfSprites);
#endif

            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Modal for Export Current Sprite
        if (ImGui::BeginPopupModal(modalName[2], NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (modal_return_status > 0)
#ifdef __MINGW64__
                ImGui::Text("Sprite[%lld] exported successfully.", spr_idx);
#else
                ImGui::Text("Sprite[%ld] exported successfully.", spr_idx);
#endif
            else
                ImGui::Text("Failed to export current sprite.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Modal for Export as Sprite Atlas
        if (ImGui::BeginPopupModal(modalName[3], NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (modal_return_status == 0)
                ImGui::Text("Sprite Atlas exported successfully.");
            else
                ImGui::Text("Failed to export sprite atlas.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Modal for Export Sprite Database
        if (ImGui::BeginPopupModal(modalName[4], NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (modal_return_status == 0)
                ImGui::Text("Sprite database exported successfully.");
            else
                ImGui::Text("Failed to export sprite atlas.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Modal for showing sprite statistics
        if (ImGui::BeginPopupModal(modalName[5], NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            showSpriteStatistics(sff, getSpriteStatistics(sff));
            ImGui::EndPopup();
        }

        if (spr_idx >= sff.header.NumberOfSprites)
            spr_idx = sff.header.NumberOfSprites - 1;
        if (spr_idx < 0)
            spr_idx = 0;
        Sprite& s = sff.sprites[spr_idx];

        ImGui::Begin("Active Sprite");
#ifdef __MINGW64__
        ImGui::Text("No: %lld", spr_idx);
#else
        ImGui::Text("No: %ld", spr_idx);
#endif

        ImGui::Text("Group: %d,%d", s.Group, s.Number);
        ImGui::Text("Size: %dx%d", s.Size[0], s.Size[1]);
        // ImGui::Text("Compression: %s", sff.header.Ver0 == 2 ? compression_code[-s.rle].c_str() : "PCX");
        ImGui::Text("Compression: %s", compression_format_code[s.rle].c_str());
        if (sff.header.Ver0 == 2) {
            ImGui::Text("Color depth: %d", s.coldepth);
        }
        ImGui::Text("Palette No: %d", s.palidx);
        if (opt_palettes.size())
            ImGui::Checkbox("Use Additional Palette", &useOptPalette);
        // ImGui::Text("Rendering %.1f fps", io.Framerate);
        // ImGui::SliderFloat("Zoom", &spr_zoom, 0.1f, 10.0f);
        ImGui::End();

        if (opt_palettes.size()) {
            ImGui::Begin("Additional Palettes");
            if (ImGui::BeginListBox("##pal_listbox", ImVec2(-FLT_MIN, -FLT_MIN))) {
                for (size_t i = 0; i < opt_palette_paths.size(); ++i) {
                    const bool isSelected = (o_palidx == i);
                    if (ImGui::Selectable(getFilename(opt_palette_paths[i].c_str()), isSelected)) {
                        o_palidx = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::End();
        }

        ImGui::Begin("Help");
        ImGui::Text("Use mouse wheel to browse sprite");
        ImGui::Text("Hold mouse right button and use mouse wheel to zoom");
        ImGui::Text("Press SPACE to start/stop auto animation");
        ImGui::Text("Press HOME to go to first sprite");
        ImGui::Text("Press END to go to last sprite");
        ImGui::Text("Press ESC or Q to quit");
        ImGui::End();

        ImGui::Begin("Image Preview");
        // Get window info
        ImVec2 avail_size = ImGui::GetContentRegionAvail();
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 cursor_pos = ImGui::GetCursorPos();
        ImVec2 draw_pos = ImVec2(window_pos.x + cursor_pos.x, window_pos.y + cursor_pos.y);

        // Handle mouse wheel zoom inside Image Preview
        if (ImGui::IsWindowHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            float wheel = io.MouseWheel; // +1 up, -1 down
            if (wheel != 0.0f) {
                if (io.MouseDown[1]) {
                    float zoomSpeed = 0.1f;
                    spr_zoom += -wheel * zoomSpeed;
                    if (spr_zoom < 0.1f) spr_zoom = 0.1f; // clamp minimum zoom
                    if (spr_zoom > 10.0f) spr_zoom = 10.0f; // clamp maximum zoom
                } else {
                    if (wheel > 0) spr_idx--; else spr_idx++; // next / prev sprite
                }
            }
        }

        float max_width = avail_size.x / s.Size[0];
        float max_height = avail_size.y / s.Size[1];
        float max_zoom = (s.Size[0] > s.Size[1]) ? max_width : max_height;

        if (spr_zoom > max_zoom || spr_zoom < 1.0f) {
            spr_zoom = (max_zoom < 1.0f) ? max_zoom : 1.0f;
        }

        // Compute scaled size
        float scaledWidth = s.Size[0] * spr_zoom;
        float scaledHeight = s.Size[1] * spr_zoom;

        // Center position based on zoom
        float offset_x = (avail_size.x - scaledWidth) * 0.5f;
        float offset_y = (avail_size.y - scaledHeight) * 0.5f;
        draw_pos.x += offset_x;
        draw_pos.y += offset_y;

        // Reserve layout space
        ImGui::Dummy(avail_size);

        // Show zoom percentage
        char buf[32];
        snprintf(buf, sizeof(buf), "Zoom: %.0f%%", spr_zoom * 100.0f);
        ImVec2 text_size = ImGui::CalcTextSize(buf);

        // New: Draw text at left-bottom corner
        ImVec2 text_pos = ImVec2(ImGui::GetWindowPos().x + 5,
            ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - text_size.y - 5);

        // Optional: dark background behind text
        ImVec2 padding = ImVec2(4, 2);
        ImVec2 rect_min = ImVec2(text_pos.x - padding.x, text_pos.y - padding.y);
        ImVec2 rect_max = ImVec2(text_pos.x + text_size.x + padding.x, text_pos.y + text_size.y + padding.y);

        ImGui::GetForegroundDrawList()->AddRectFilled(rect_min, rect_max, IM_COL32(0, 0, 100, 255), 4.0f);
        ImGui::GetForegroundDrawList()->AddText(text_pos, IM_COL32(255, 255, 255, 255), buf);
        ImGui::End();

        // ImGui Rendering
        ImGui::Render();
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Custom Sprite Rendering
        if (useOptPalette) {
            renderSprite(s, opt_palettes[o_palidx].texture_id, draw_pos.x, draw_pos.y, spr_zoom);
        } else {
            renderSprite(s, sff.palettes[s.palidx].texture_id, draw_pos.x, draw_pos.y, spr_zoom);
        }

        SDL_GL_SwapWindow(window);
    }

    // Clean up
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &g_quadVAO);
    glDeleteBuffers(1, &g_quadVBO);
    glDeleteProgram(g_RGBAShaderProgram);
    glDeleteProgram(g_PalettedShaderProgram);

    deleteMugenSprite(sff);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
