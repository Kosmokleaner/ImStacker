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
"in vec4 vertexColor;\n"
"\n"
"void main()\n"
"{\n"
"    FragColor = vertexColor; \n";

const char* fragment_shader_text1 =
"}\n"
"\n";

static GLuint vertex_shader = 0;
static GLuint g_program = 0;
static GLuint g_vbo = 0;
static GLuint g_idx = 0;

void genShaderCode(const char* inCode) {
    assert(inCode);
    std::string temp;

    temp = fragment_shader_text0;
    temp += inCode;
    temp += fragment_shader_text1;

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* ptr = temp.data();
    glShaderSource(fragment_shader, 1, &ptr, NULL);
    glCompileShader(fragment_shader);

    g_program = glCreateProgram();
    glAttachShader(g_program, vertex_shader);
    glAttachShader(g_program, fragment_shader);
    glLinkProgram(g_program);
}

static void init() {
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);


    genShaderCode("");

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
