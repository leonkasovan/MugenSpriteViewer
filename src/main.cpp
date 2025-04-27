// Optimized SDL2 + OpenGL Sprite Renderer using Uniforms (Modern Way)
// Author: ChatGPT + You (awesome team!)

#include "mugen_sff.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "glad.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif


#define Window_w 640
#define Window_h 480

// Global variable
GLuint shaderProgram;
GLuint quadVAO, quadVBO;
GLint texLocation, paletteLocation, positionLocation, sizeLocation, windowSizeLocation;

std::map<int, std::string> compression_code = {
    {1, "PCX"},
    {2, "RLE8"},
    {3, "RLE5"},
    {4, "LZ5"},
    {10, "PNG10"},
    {11, "PNG11"},
    {12, "PNG12"}
};

const char* vertexShaderSource = R"(
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

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D tex;
uniform sampler2D paletteTex;
void main() {
    FragColor = texture(paletteTex, vec2(texture(tex, TexCoord).r, 0.5));
})";

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

GLuint createShaderProgram() {
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

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// GLuint generatePaletteTexture() {
//     GLuint tex;
//     float palette[256 * 4];
//     for (int i = 0; i < 256; ++i) {
//         palette[i * 4 + 0] = 0.0f;
//         palette[i * 4 + 1] = 0.0f;
//         palette[i * 4 + 2] = i / 255.0f;
//         palette[i * 4 + 3] = i?0.0f:1.0f;
//     }

//     glGenTextures(1, &tex);
//     glBindTexture(GL_TEXTURE_2D, tex);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_FLOAT, palette);

//     return tex;
// }

// GLuint generateTexture(int width, int height) {
//     GLuint tex;
//     unsigned char* data = (unsigned char*)malloc(width * height);
//     for (int i = 0; i < width * height; ++i)
//         data[i] = (170 - width) + (i % width);

//     glGenTextures(1, &tex);
//     glBindTexture(GL_TEXTURE_2D, tex);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);

//     free(data);
//     return tex;
// }

// void renderSprite(GLuint texture, GLuint paletteTex, float x, float y, float width, float height, int windowWidth, int windowHeight) {
void renderSprite(Sprite &spr, GLuint paletteTex, float x, float y) {
    glUseProgram(shaderProgram);
    glBindVertexArray(quadVAO);

    glUniform2f(positionLocation, x, y);
    glUniform2f(sizeLocation, spr.Size[0], spr.Size[1]);
    glUniform2f(windowSizeLocation, Window_w, Window_h);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, spr.texture_id);
    glUniform1i(texLocation, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, paletteTex);
    glUniform1i(paletteLocation, 1);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

int main(int argc, char*argv[]) {
    if (argc <= 1) {
        printf("MugenSpriteViewer\nUsage: %s [filenname]\n", argv[0]);
        return -1;
    }

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
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
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Mugen Sprite Viewer by leonkasovan@gmail.com", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Window_w, Window_h, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Load OpenGL Extention via glad
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    shaderProgram = createShaderProgram();
    setupQuad();

    texLocation = glGetUniformLocation(shaderProgram, "tex");
    paletteLocation = glGetUniformLocation(shaderProgram, "paletteTex");
    positionLocation = glGetUniformLocation(shaderProgram, "uPosition");
    sizeLocation = glGetUniformLocation(shaderProgram, "uSize");
    windowSizeLocation = glGetUniformLocation(shaderProgram, "uWindowSize");

    // Sprite Global Variable
    Sff sff;
    int64_t spr_idx = 0;   // Sprite index to be displayed

    // Generating Sprite's Texture and Palette's Texture from SFF file
    if (loadMugenSprite(argv[1], &sff) != 0){
        fprintf(stderr, "Failed to load Mugen Sprite %s\n", argv[1]);
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
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
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_SPACE:
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
                    case SDLK_END:
                        spr_idx = sff.header.NumberOfSprites - 1;
                        break;
                    default:
                        break;
                }

                if (spr_idx>=sff.header.NumberOfSprites)
                    spr_idx = sff.header.NumberOfSprites - 1;
                if (spr_idx<0)
                    spr_idx = 0;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            // static float f = 0.0f;
            // static int counter = 0;

            ImGui::Begin("Mugen Sprite Information");
            ImGui::Text("Filename: %s", sff.filename);
            ImGui::Text("Version: %d.%d.%d.%d", sff.header.Ver0, sff.header.Ver1, sff.header.Ver2, sff.header.Ver3);
            ImGui::Text("Total Sprites: %u", sff.header.NumberOfSprites);
            ImGui::Text("Total Palettes: %u", sff.header.NumberOfPalettes);
            // ImGui::Checkbox("Another Window", &show_another_window);

            // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            // ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            // if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            //     counter++;
            // ImGui::SameLine();
            // ImGui::Text("counter = %d", counter);

            ImGui::Text("Rendering %.1f fps", io.Framerate);
            ImGui::End();

            Sprite &s = sff.sprites[spr_idx];
            ImGui::Begin("Active Sprite");
            ImGui::Text("Index: %ld (%d,%d)", spr_idx, s.Group, s.Number);
            ImGui::Text("Size: %dx%d", s.Size[0], s.Size[1]);
            ImGui::Text("Palette: %d", s.palidx);
            ImGui::Text("Compression: %s", sff.header.Ver0 == 2 ? compression_code[-s.rle].c_str() : "PCX");
            if (sff.header.Ver0 == 2) {
                ImGui::Text("Color depth: %d", s.coldepth);
            }
            ImGui::End();
        }

        // 3. Show another simple window.
        // if (show_another_window)
        // {
        //     ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        //     ImGui::Text("Hello from another window!");
        //     if (ImGui::Button("Close Me"))
        //         show_another_window = false;
        //     ImGui::End();
        // }

        // ImGui Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        Sprite &s = sff.sprites[spr_idx];
        renderSprite(s, sff.palettes[s.palidx].texture_id, 400 - s.Offset[0], 240 - s.Offset[1]);

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteProgram(shaderProgram);
    
    deleteMugenSprite(sff);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
