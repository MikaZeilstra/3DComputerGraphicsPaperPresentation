// Disable compiler warnings in third-party code (which we cannot change).
#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cassert>
#include <cstdlib> // EXIT_FAILURE
#include <framework/mesh.h>
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <iostream>
#include <numeric>
#include <optional>
#include <span>
#include <vector>
#include <time.h>
#include <string>

// Configuration
const int WIDTH = 800;
const int HEIGHT = 800;
float refractionIndex = 1.01; //glass

bool debug_textures = false;
bool use_dragon = true;
bool show_background = true;
bool show_model = true;
int texture_type = 0;

float prev_time = clock();



// Program entry point. Everything starts here.
int main(int argc, char** argv)
{
    Window window { "Shading", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL45 };
    Trackball trackball { &window, glm::radians(50.0f) };

    const Mesh meshSphere = loadMesh(argc == 2 ? argv[1] : "resources/sphere.obj")[0];
    const Mesh meshDragon = loadMesh(argc == 2 ? argv[1] : "resources/dragon.obj")[0];
    const Mesh meshCube = loadMesh(argc == 2 ? argv[1] : "resources/Cube.obj")[0];


    window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
        if (action != GLFW_RELEASE)
            return;
        if (key == GLFW_KEY_D)
            debug_textures = !debug_textures;
        if (key == GLFW_KEY_F)
            texture_type = ((texture_type + 1) % 3);
        if (key == GLFW_KEY_UP)
            refractionIndex += 0.01;
        if (key == GLFW_KEY_DOWN)
            refractionIndex -= 0.01;
        if (key == GLFW_KEY_LEFT)
            refractionIndex = 1.01;
        if (key == GLFW_KEY_RIGHT)
            refractionIndex = 1.5;
        if (key == GLFW_KEY_M)
            show_model = !show_model;
        if (key == GLFW_KEY_B)
            show_background = !show_background;
        if (key == GLFW_KEY_S)
            use_dragon = !use_dragon;
        });
    const Shader debugShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, "shaders/debug_frag.glsl").build();
    const Shader BackFaceShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, "shaders/BackFace.glsl").build();
    const Shader EnviromentShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, "shaders/EnviromentMap.glsl").build();
    const Shader texturedebugShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, "shaders/texDebug.glsl").addStage(GL_FRAGMENT_SHADER, "shaders/tex_debug_frag.glsl").build();
    const Shader refractionShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, "shaders/refraction.glsl").build();

    //Create Vbo for debug Quad
    std::vector<Vertex> texQuadvertex = { {{-1,-1,-1},{0,0,0},{0,0}},{{1,-1,-1},{0,0,0},{1,0}},{{-1,1,-1},{0,0,0},{0,1}},{{1,-1,-1},{0,0,0},{1,0}},{{1,1,-1},{0,0,0},{1,1}},{{-1,1,-1},{0,0,0},{0,1}},{{0,0,0},{0,0,0},{0,0}} };
    std::vector<glm::uvec3> texQuadIndex = { {0,1,2},{3,4,5},{6,6,6} };
    
    GLuint vboQuad;
    glCreateBuffers(1, &vboQuad);
    glNamedBufferStorage(vboQuad, static_cast<GLsizeiptr>(texQuadvertex.size() * sizeof(Vertex)), texQuadvertex.data(), 0);

    GLuint iboQuad;
    glCreateBuffers(1, &iboQuad);
    glNamedBufferStorage(iboQuad, static_cast<GLsizeiptr>(texQuadIndex.size() * sizeof(glm::uvec3)), texQuadIndex.data(), 0);

    // Bind vertex data to shader inputs using their index (location).
    // These bindings are stored in the Vertex Array Object.
    GLuint vaoQuad;
    glCreateVertexArrays(1, &vaoQuad);

    // The indices (pointing to vertices) should be read from the index buffer.
    glVertexArrayElementBuffer(vaoQuad, iboQuad);


    // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
    // The stride is the distance in bytes between vertices. We use the offset to point to the normals
    // instead of the positions.
    glVertexArrayVertexBuffer(vaoQuad, 0, vboQuad, offsetof(Vertex, position), sizeof(Vertex));
    glVertexArrayVertexBuffer(vaoQuad, 1, vboQuad, offsetof(Vertex, normal), sizeof(Vertex));
    glVertexArrayVertexBuffer(vaoQuad, 2, vboQuad, offsetof(Vertex, texCoord), sizeof(Vertex));
    glEnableVertexArrayAttrib(vaoQuad, 0);
    glEnableVertexArrayAttrib(vaoQuad, 1);
    glEnableVertexArrayAttrib(vaoQuad, 2);

    
    // Create Cube Vertex Buffer Object and Index Buffer Objects.
    GLuint vboCube;
    glCreateBuffers(1, &vboCube);
    glNamedBufferStorage(vboCube, static_cast<GLsizeiptr>(meshCube.vertices.size() * sizeof(Vertex)), meshCube.vertices.data(), 0);

    GLuint iboCube;
    glCreateBuffers(1, &iboCube);
    glNamedBufferStorage(iboCube, static_cast<GLsizeiptr>(meshCube.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), meshCube.triangles.data(), 0);

    // Bind vertex data to shader inputs using their index (location).
    // These bindings are stored in the Vertex Array Object.
    GLuint vaoCube;
    glCreateVertexArrays(1, &vaoCube);

    // The indices (pointing to vertices) should be read from the index buffer.
    glVertexArrayElementBuffer(vaoCube, iboCube);


    // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
    // The stride is the distance in bytes between vertices. We use the offset to point to the normals
    // instead of the positions.
    glVertexArrayVertexBuffer(vaoCube, 0, vboCube, offsetof(Vertex, position), sizeof(Vertex));
    glVertexArrayVertexBuffer(vaoCube, 1, vboCube, offsetof(Vertex, normal), sizeof(Vertex));
    glVertexArrayVertexBuffer(vaoCube, 2, vboCube, offsetof(Vertex, texCoord), sizeof(Vertex));
    glEnableVertexArrayAttrib(vaoCube, 0);
    glEnableVertexArrayAttrib(vaoCube, 1);
    glEnableVertexArrayAttrib(vaoCube, 2);
    

    // Create Dragon Vertex Buffer Object and Index Buffer Objects.
    GLuint vboDragon;
    glCreateBuffers(1, &vboDragon);
    glNamedBufferStorage(vboDragon, static_cast<GLsizeiptr>(meshDragon.vertices.size() * sizeof(Vertex)), meshDragon.vertices.data(), 0);

    GLuint iboDragon;
    glCreateBuffers(1, &iboDragon);
    glNamedBufferStorage(iboDragon, static_cast<GLsizeiptr>(meshDragon.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), meshDragon.triangles.data(), 0);

    // Bind vertex data to shader inputs using their index (location).
    // These bindings are stored in the Vertex Array Object.
    GLuint vaoDragon;
    glCreateVertexArrays(1, &vaoDragon);

    // The indices (pointing to vertices) should be read from the index buffer.
    glVertexArrayElementBuffer(vaoDragon, iboDragon);


    // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
    // The stride is the distance in bytes between vertices. We use the offset to point to the normals
    // instead of the positions.
    glVertexArrayVertexBuffer(vaoDragon, 0, vboDragon, offsetof(Vertex, position), sizeof(Vertex));
    glVertexArrayVertexBuffer(vaoDragon, 1, vboDragon, offsetof(Vertex, normal), sizeof(Vertex));
    glEnableVertexArrayAttrib(vaoDragon, 0);
    glEnableVertexArrayAttrib(vaoDragon, 1);

    // Create Dragon Vertex Buffer Object and Index Buffer Objects.
    GLuint vboSphere;
    glCreateBuffers(1, &vboSphere);
    glNamedBufferStorage(vboSphere, static_cast<GLsizeiptr>(meshSphere.vertices.size() * sizeof(Vertex)), meshSphere.vertices.data(), 0);

    GLuint iboSphere;
    glCreateBuffers(1, &iboSphere);
    glNamedBufferStorage(iboSphere, static_cast<GLsizeiptr>(meshSphere.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), meshSphere.triangles.data(), 0);

    // Bind vertex data to shader inputs using their index (location).
    // These bindings are stored in the Vertex Array Object.
    GLuint vaoSphere;
    glCreateVertexArrays(1, &vaoSphere);

    // The indices (pointing to vertices) should be read from the index buffer.
    glVertexArrayElementBuffer(vaoSphere, iboSphere);


    // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
    // The stride is the distance in bytes between vertices. We use the offset to point to the normals
    // instead of the positions.
    glVertexArrayVertexBuffer(vaoSphere, 0, vboSphere, offsetof(Vertex, position), sizeof(Vertex));
    glVertexArrayVertexBuffer(vaoSphere, 1, vboSphere, offsetof(Vertex, normal), sizeof(Vertex));
    glEnableVertexArrayAttrib(vaoSphere, 0);
    glEnableVertexArrayAttrib(vaoSphere, 1);


    // Load image from disk to CPU memory.
    int width, height, sourceNumChannels; // Number of channels in source image. pixels will always be the requested number of channels (3).
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels = stbi_load("resources/CubeMap.png", &width, &height, &sourceNumChannels, STBI_rgb_alpha);
    
    // Create a texture on the GPU with 4 channels with 8 bits each.
    GLuint cubemapTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &cubemapTexture);
    glTextureStorage2D(cubemapTexture, 1, GL_RGBA8, width, height);

    // Upload pixels into the GPU texture.
    glTextureSubImage2D(cubemapTexture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    // Free the CPU memory after we copied the image to the GPU.
    stbi_image_free(pixels);

    // Set behavior for when texture coordinates are outside the [0, 1] range.
    glTextureParameteri(cubemapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
    glTextureParameteri(cubemapTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(cubemapTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_set_flip_vertically_on_load(false);
    stbi_uc* posx = stbi_load("resources/posx.jpg", &width, &height, &sourceNumChannels, STBI_rgb_alpha);
    stbi_uc* negx = stbi_load("resources/negx.jpg", &width, &height, &sourceNumChannels, STBI_rgb_alpha);
    stbi_uc* posy = stbi_load("resources/posy.jpg", &width, &height, &sourceNumChannels, STBI_rgb_alpha);
    stbi_uc* negy = stbi_load("resources/negy.jpg", &width, &height, &sourceNumChannels, STBI_rgb_alpha);
    stbi_uc* posz = stbi_load("resources/posz.jpg", &width, &height, &sourceNumChannels, STBI_rgb_alpha);
    stbi_uc* negz = stbi_load("resources/negz.jpg", &width, &height, &sourceNumChannels, STBI_rgb_alpha);


    GLuint envMapTex;
    glGenTextures(1, &envMapTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envMapTex);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, posx);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, negx);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, posy);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, negy);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, posz);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, negz);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    //setup new framebuffer for refraction
    unsigned int refractionFrameBuffer;
    glGenFramebuffers(1, &refractionFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFrameBuffer);

    GLuint refractionDepthrenderbuffer;
    glGenRenderbuffers(1, &refractionDepthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, refractionDepthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.getWindowSize().x, window.getWindowSize().y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, refractionDepthrenderbuffer);


    //Setup normal texture
    GLuint backFaceNormalsTexture;
    glGenTextures(1, &backFaceNormalsTexture);
    glBindTexture(GL_TEXTURE_2D, backFaceNormalsTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window.getWindowSize().x, window.getWindowSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, backFaceNormalsTexture, 0);
    

    //Setup Depth texture
    GLuint backFaceDepthTexture;
    glGenTextures(1, &backFaceDepthTexture);
    glBindTexture(GL_TEXTURE_2D, backFaceDepthTexture);
        
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, window.getWindowSize().x, window.getWindowSize().y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, backFaceDepthTexture, 0);
    

    // Enable depth testing.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_BLEND);
    
    // Main loop.
    while (!window.shouldClose()) {
        window.updateInput();
        float fps = 1/((clock() - prev_time)/ CLOCKS_PER_SEC);
        prev_time = clock();
        
        glfwSetWindowTitle(window.m_pWindow, std::to_string(fps).c_str());

        // Set model/view/projection matrix.
        const glm::vec3 cameraPos = trackball.position();
        const glm::mat4 model{ 1.0f };
        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();
        const glm::mat4 mvp = projection * view * model;
        const glm::mat4 projection_inv = glm::inverse(projection);
        
        
        //Render the backface normals and z-buffer to textures
        glBindFramebuffer(GL_FRAMEBUFFER, refractionFrameBuffer);

        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        BackFaceShader.bind();
        if(use_dragon)
            glBindVertexArray(vaoDragon);
        else
            glBindVertexArray(vaoSphere);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3fv(1, 1, glm::value_ptr(cameraPos));

        if(use_dragon)
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshDragon.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);
        else
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshSphere.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

        glDisable(GL_CULL_FACE);

       
        //Bind the window framebuffer again
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        //If in texture debugmode show the textures
        if (debug_textures) {
            //Set debug shader
            texturedebugShader.bind();
            //Bind the quad VAO and render
            glBindVertexArray(vaoQuad);

            //Activate correct texture
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(1, 0);
            if (texture_type == 0) {
                glBindTexture(GL_TEXTURE_2D, backFaceNormalsTexture);
            }
            else if (texture_type == 1) {
                glBindTexture(GL_TEXTURE_2D, backFaceDepthTexture);
            }
            else if (texture_type == 2) {
                glBindTexture(GL_TEXTURE_2D, cubemapTexture);
            }
            else if (texture_type == 3) {

            }
            else if (texture_type == 4) {
                
            }
           
                     
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(2) * 3, GL_UNSIGNED_INT, nullptr);
        }
        else {
            if (show_background) {
                //Render the environment as background
                EnviromentShader.bind();
                glBindVertexArray(vaoCube);

                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, envMapTex);
                glUniform1i(2, 1);
                glUniform3fv(3, 1, glm::value_ptr(cameraPos));

                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshCube.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);
            }

            if (show_model) {
                //Render refractive model
                refractionShader.bind();
                if (use_dragon) {
                    glBindVertexArray(vaoDragon);
                }
                else {
                    glBindVertexArray(vaoSphere);
                }


                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
                glUniform3fv(1, 1, glm::value_ptr(cameraPos));

                glUniform1f(2, refractionIndex);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, backFaceNormalsTexture);
                glUniform1i(3, 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, backFaceDepthTexture);
                glUniform1i(4, 1);

                glUniformMatrix4fv(7, 1, GL_FALSE, glm::value_ptr(projection_inv));
                glActiveTexture(GL_TEXTURE4);
                glUniform1i(8, 4);
                glBindTexture(GL_TEXTURE_CUBE_MAP, envMapTex);

                if (use_dragon) {
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshDragon.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);
                }
                else {
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshSphere.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);
                }
            }
        }
        
        // Present result to the screen.
        window.swapBuffers();
    }

    // Be a nice citizen and clean up after yourself.
    glDeleteBuffers(1, &vboDragon);
    glDeleteBuffers(1, &iboDragon);
    glDeleteVertexArrays(1, &vaoDragon);

    return 0;
}