#include "imgui.h"
#include <string>
#include <imgui_impl_opengl3_loader.h>

// https://www.khronos.org/registry/OpenGL/api/GL/glext.h
#define GL_STATIC_DRAW                    0x88E4

const char* vertex_shader_text =
"#version 330 core\n"
"layout(location = 0) in vec3 aPos; // the position variable has attribute position 0\n"
"\n"
"out vec4 vertexColor;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(aPos, 1.0);\n"
"    vertexColor = vec4(0.5, 0.0, 0.0, 1.0);\n"
"}\n"
"\n";

const char* fragment_shader_text0 =
"#version 330 core\n"
"out vec4 FragColor; \n"
"\n"
"in vec4 vertexColor; // red\n"
"in vec4 gl_FragCoord; // (pixel.x+0.5, pixel.y+0.5, z, w)\n"
"\n"
"void main()\n"
"{\n"
"   FragColor = vertexColor;\n";

const char* fragment_shader_text1 =
"}\n"
"\n";

static GLuint vertex_shader = 0;
static GLuint g_program = 0;
static GLuint g_vbo = 0;
static GLuint g_idx = 0;

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
// @param whatToCheck GL_LINK_STATUS for program or GL_COMPILE_STATUS for shaders
static bool checkProgram(GLuint handle, GLuint whatToCheck, const char* desc, std::string& warningsAndErrors) {
    warningsAndErrors.clear();

    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, whatToCheck, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE) {
        OutputDebugStringA("ERROR: ");
        OutputDebugStringA(desc);
        OutputDebugStringA("\n");
    }
    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
        OutputDebugStringA(buf.begin());
        warningsAndErrors = buf.begin();
    }
    return (GLboolean)status == GL_TRUE;
}

void recompileShaders(const char* inCode, std::string& warningsAndErrors) {
    assert(inCode);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* ptr = inCode;
    glShaderSource(fragment_shader, 1, &ptr, NULL);
    glCompileShader(fragment_shader);

    g_program = glCreateProgram();
    glAttachShader(g_program, vertex_shader);
    glAttachShader(g_program, fragment_shader);
    glLinkProgram(g_program);
    checkProgram(g_program, GL_LINK_STATUS, "program", warningsAndErrors);
}

static void init() {
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    std::string warningsAndErrors;
    std::string shaderCode;
    shaderCode += fragment_shader_text0;
    shaderCode += fragment_shader_text1;
    recompileShaders(shaderCode.c_str(), warningsAndErrors);

    const float r = 1.0f;

    // x, y, z
    GLfloat vertices[] = {
      -r,  r,  0.5f, // 0, left top
       r,  r,  0.5f, // 1, right, top
      -r, -r,  0.5f, // 2, left bottom
       r, -r,  0.5f, // 3, right bottom
    };

    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    unsigned int indices[] = {
        0, 1, 2,
        1, 3, 2
    };

    glGenBuffers(1, &g_idx);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_idx);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

static void deinit() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &g_idx);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &g_vbo);
}

void drawDemo() {
    static bool first = true;

    if (first) {
        init();
        first = false;
    }

//    ImDrawData* draw_data = ImGui::GetDrawData();
//    MyImGuiRenderFunction(draw_data);

//    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//    glClear(GL_COLOR_BUFFER_BIT);
//    glViewport(0, 0, 100, 100);
    glScissor(0, 0, 16*1024, 16 * 1024);
    glUseProgram(g_program);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_idx);
    glVertexAttribPointer(
        0, // attribute pos 0
        3, GL_FLOAT, // 3 floats
        GL_FALSE, // sRGB ?
        3 * sizeof(float), // vertex buffer stride in bytes
        (void*)0 // byte offset in buffer
        );
    glEnableVertexAttribArray(0);
//    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    glUseProgram(0);
//    ImGui::Begin("Hello GLSL");                          // Create a window called "Hello, world!" and append into it.
//    ImGui::Text("Test");
//    ImGui::End();
}
