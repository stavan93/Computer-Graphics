// Splat-based point renderer.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, rotate
#include <glm/gtx/string_cast.hpp> // glm::to_string()

#include "mesh.hpp"
#include "shader.hpp"
#include "window.hpp"
#include "camera.hpp"
#include "grid.hpp"
#include "flat_lighter.hpp"
#include "mesh.hpp"

#define PI 3.14159265

using namespace std;

#define EPSILON 0.0001

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback( GLFWwindow* window, int button,
                            int action, int mods );
void mouse_motion_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window);

Camera cam;
bool scene_changed = true;
GLFWwindow *window;
Grid grid;
Flat_Lighter lighter;
float vertex_size = 0.1;

float vertices;
float edges;
float x_vertices[100];
float y_vertices[100];

int source[20];
int destination[20];

glm::vec3 world;
float mouse_world_x;
float mouse_world_y;

vector<Mesh *> vertex_shape;
vector<Mesh*> edge_shape;
vector<Mesh*> int_shape;

bool isActive;
int active_vertex;
int intersection_count;

int add_vertex(float x, float y, float z,
               float *coords, GLuint *indexes, int vert_index) {
    int i = vert_index / 4;
    indexes[i] = i;
    coords[vert_index++] = x;
    coords[vert_index++] = y;
    coords[vert_index++] = z;
    coords[vert_index++] = 1;
    return vert_index;
}

void make_scene() {
    intersection_count = 0;
    vertex_shape.clear();
    edge_shape.clear();
    int_shape.clear();
    int max_vertices = 100;
    float *coords = new float[max_vertices * 4];
    GLuint *indexes = new GLuint[max_vertices];
    GLuint program_handle = lighter.get_program_handle();

    for (int j = 0; j < vertices; j++) {
        int i = 0;
        float s = vertex_size / 2;
        i = add_vertex(x_vertices[j] - 0.5 - s, y_vertices[j] - 0.5 - s, 0, coords, indexes, i);
        i = add_vertex(x_vertices[j] - 0.5 + s, y_vertices[j] - 0.5 - s, 0, coords, indexes, i);
        i = add_vertex(x_vertices[j] - 0.5 + s, y_vertices[j] - 0.5 + s, 0, coords, indexes, i);
        i = add_vertex(x_vertices[j] - 0.5 - s, y_vertices[j] - 0.5 + s, 0, coords, indexes, i);
        vertex_shape.push_back(new Mesh(coords, 4, 4,
            indexes, 4,
            4,
            GL_TRIANGLE_FAN, program_handle,
            "Vertex"));
    }

    for (int j = 0; j < edges; j++) {
        int i = 0;
        float s = vertex_size / 2;
        i = add_vertex(x_vertices[source[j]]-0.5, y_vertices[source[j]]-0.5, 0, coords, indexes, i);
        i = add_vertex(x_vertices[destination[j]]-0.5, y_vertices[destination[j]]-0.5, 0, coords, indexes, i);
        edge_shape.push_back(new Mesh(coords, 2, 4,
            indexes, 2,
            2,
            GL_LINE_STRIP, program_handle,
            "Edge"));
    }

    for (int j = 0; j < edges; j++) {
        for (int k = 0; k < edges; k++) {
            if (j != k) {
                float a = (x_vertices[destination[j]] - 0.5) - (x_vertices[source[j]]-0.5);
                float b = (x_vertices[source[k]]-0.5) - (x_vertices[destination[k]]-0.5);
                float c = (x_vertices[source[k]]-0.5) - (x_vertices[source[j]]-0.5);

                float d = (y_vertices[destination[j]] - 0.5) - (y_vertices[source[j]] - 0.5);
                float e = (y_vertices[source[k]] - 0.5) - (y_vertices[destination[k]] - 0.5);
                float f = (y_vertices[source[k]] - 0.5) - (y_vertices[source[j]] - 0.5);

                float delta = (a * e) - (b * d);
                if (delta != 0.0) {
                    float delta_x = (c * e) - (b * f);
                    float delta_y = (a * f) - (c * d);

                    float t = delta_x / delta;
                    float u = delta_y / delta;

                    if (((t > EPSILON && t < 1 - EPSILON) && (u > EPSILON && u < 1 - EPSILON))) {
                        float P_x = (x_vertices[source[j]] - 0.5) + t * ((x_vertices[destination[j]] - 0.5) - (x_vertices[source[j]] - 0.5));
                        float P_y = (y_vertices[source[j]] - 0.5) + t * ((y_vertices[destination[j]] - 0.5) - (y_vertices[source[j]] - 0.5));

                        int i = 0;
                        float s = 0.05 / 2;
                        i = add_vertex(P_x - s, P_y - s, 0, coords, indexes, i);
                        i = add_vertex(P_x + s, P_y - s, 0, coords, indexes, i);
                        i = add_vertex(P_x + s, P_y + s, 0, coords, indexes, i);
                        i = add_vertex(P_x - s, P_y + s, 0, coords, indexes, i);
                        int_shape.push_back(new Mesh(coords, 4, 4,
                            indexes, 4,
                            4,
                            GL_TRIANGLE_FAN, program_handle,
                            "Intersection"));

                        intersection_count += 1;
                    }
                }
            }
        }
    }
    delete[] coords;
    delete[] indexes;
}


void draw_scene() {
    // render
    // ------
    if (intersection_count == 0) {
        glClearColor(0,1,0,0);
    }
    else {
        glClearColor(0.4f, 0.2f, 0.1f, 1.0f); // dark red/brown
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    grid.draw();

    glm::vec3 position(0.5, 0.5, 0);
    glm::mat4 move = glm::translate(glm::mat4(1.0f), position);
    lighter.set_model_transform(move);

    //vertex_shape[0]->draw();
    for (int j = 0; j < vertex_shape.size(); j++) {
        int binaryNum[32];
        int i = 0;
        if (j == 0) {
            lighter.set_color(glm::vec4(0, 0, 0, 0));
            vertex_shape[j]->draw();
        }
        else {
            int n = j;
            while (n > 0) {
                binaryNum[i] = n % 2;
                n = n / 2;
                i++;
            }
            lighter.set_color(glm::vec4(binaryNum[2], binaryNum[1], binaryNum[0], 0));
            vertex_shape[j]->draw();
        }
    }

    lighter.set_color(glm::vec4(1, 1, 0, 0));
    for (int j = 0; j < edges; j++) {
        edge_shape[j]->draw();
    }

    lighter.set_color(glm::vec4(1, 1, 1, 1));
    for (int j = 0; j < int_shape.size(); j++) {
        int_shape[j]->draw();
    }
    
    vertex_shape.clear();
    edge_shape.clear();
    int_shape.clear();

    for (Mesh* mesh : vertex_shape) {
        delete mesh;
    }

    for (Mesh* mesh : edge_shape) {
        delete mesh;
    }

    for (Mesh* mesh : int_shape) {
        delete mesh;
    }

    glfwSwapBuffers(window);
    scene_changed = false;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage:\n";
        std::cerr << "  uncross <graph-file.txt>\n";
        std::cerr << "Press ENTER to exit program:";
        char reply[10];
        std::cin.getline(reply, 9);
        exit(1);
    }
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();

    int display_width = 800;
    int display_height = 800;

    GLFW_OpenGL_Window glfw_window(3, 3,
                                   display_width,
                                   display_height,
                                   "Uncross",
                                   &framebuffer_size_callback);
    window = glfw_window.get_window();

    lighter.init();

    grid = Grid(-1, -1, 4, 4,
                1, 0.1, lighter);

    cam = Camera(display_width, display_height);
    cam.init_2D(-1.5, -1.5, 3, 3);

    glfwGetFramebufferSize(glfw_window.get_window(),
                           &display_width,
                           &display_height);
    cam.resize_display(display_width, display_height);

    lighter.set_camera(cam);

    fstream file;
    file.open(argv[1], ios::in);

    if (file.is_open()) {
        file >> vertices;
        file >> edges;

        for (int j = 0; j < vertices; j++) {
            float param = (360 / vertices) * (j + 1);
            param = (param * PI) / 180;

            x_vertices[j] = cos(param);
            y_vertices[j] = sin(param);
        }

        for (int j = 0; j < edges; j++) {
            file >> source[j];
            file >> destination[j];
        }
        file.close();
    }
    

    else {
        cout << "File not opened";
        exit(0);
    }

    make_scene();

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_motion_callback);

    // render loop
    // -----------
    scene_changed = true;
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);
        if (scene_changed) {

            draw_scene();

        }
        glfwWaitEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

bool key_pressed(unsigned int key_ID, GLFWwindow *window) {
    return glfwGetKey(window, key_ID) == GLFW_PRESS;
}

// process all input: query GLFW whether relevant keys are pressed/released
// this frame, and react accordingly
// --------------------------------------------------------------
void processInput(GLFWwindow *w)
{
    if (key_pressed(GLFW_KEY_ESCAPE, w) ||
        key_pressed(GLFW_KEY_Q, w))
        glfwSetWindowShouldClose(w, true);


}

// glfw: whenever the window size changed (by OS or user resize)
// this callback function executes
// ------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions;
    // note that width and height will be significantly larger
    // than specified on retina displays.
    cam.resize_display(width, height);
    lighter.set_camera(cam);
}

// glfw calls this every time the mouse moves
void mouse_motion_callback(GLFWwindow* window, double xpos, double ypos) {
    // Save the mouse position.
    // If a vertex is active, change its position.
    world = cam.mouse_to_world(xpos, ypos);
    mouse_world_x = world[0];
    mouse_world_y = world[1];
    if (isActive) {
        x_vertices[active_vertex] = mouse_world_x;
        y_vertices[active_vertex] = mouse_world_y;
    }
    make_scene();
    draw_scene();
    
    //cout << mouse_world_x << " " << mouse_world_y;
}

// glfw calls this every time the user presses/releases a mouse button
void mouse_button_callback( GLFWwindow* window, int button,
                            int action, int mods ) {
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    if (action == GLFW_PRESS)
    {
        // Left button was pressed:
        // 1. convert mouse coords to world.
        // 2. check each vertex, to see if the mouse
        //    is inside its square.
        // 3. if so, make the vertex active.
        float s = vertex_size / 2;
        for (int j = 0; j < vertices; j++) {
            if (mouse_world_x > (x_vertices[j] - s) && mouse_world_x < (x_vertices[j] + s)) {
                if (mouse_world_y > (y_vertices[j] - s) && mouse_world_y < (y_vertices[j] + s)) {
                    isActive = true;
                    active_vertex = j;
                    break;
                }
            }
        }
    }
    if (action == GLFW_RELEASE) {
        isActive = false;
    }
}
