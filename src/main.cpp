// Mugen Sprite (SFF) Viewer
// by leonkasovan@gmail.com, (c) 27 April 2025

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

std::map<int, std::string> compression_code = {
    {1, "PCX"},
    {2, "RLE8"},
    {3, "RLE5"},
    {4, "LZ5"},
    {10, "PNG10"},
    {11, "PNG11"},
    {12, "PNG12"}
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
    if (spr.rle == -12 || spr.rle == -11)
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


int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("MugenSpriteViewer\nUsage: %s [filename]\n", argv[0]);
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
            if (ImGui::MenuItem("Export All Sprite as PNG")) {

            }
            if (ImGui::MenuItem("Export Current Sprite as PNG")) {

            }
            ImGui::EndPopup();
        }
        ImGui::End();

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
        ImGui::Text("Compression: %s", sff.header.Ver0 == 2 ? compression_code[-s.rle].c_str() : "PCX");
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
    glDeleteProgram(g_shaderProgram);

    deleteMugenSprite(sff);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
