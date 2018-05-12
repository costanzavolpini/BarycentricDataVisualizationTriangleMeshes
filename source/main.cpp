/**
    ref: https://learnopengl.com
    glfw3: brew install glfw3
    glew: brew install glew
    just use make clean && make && ./main
    Costanza Volpini
*/

#include "Base.h"
#include "Shader.h"
#include "Arcball.h"
#include "Object.h"
#include "LoaderObject.h"
#include <math.h>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include <cstdlib>
#include "glm/ext.hpp" //to test
#include <glm/gtc/type_ptr.hpp>

#include "imgui_plot_custom.h"

#define IS_IN_DEBUG false // to show normals

using namespace std;

// settings
const unsigned int WIDTH = 1200;
const unsigned int HEIGHT = 900;

int current_width = 1200;
int current_height = 900;

GLuint frame_buffer = 0;
GLuint rendered_texture;
GLuint depth_render_buffer;

glm::mat4 view, model;

// Arcball instance
static Arcball arcball(WIDTH, HEIGHT, 1.5f, true, true);

void error_callback(int error, const char *desc);

// create object
Object object = Object();
Object horse = Object();

// Camera options
float Zoom = 45.0f;

// resize window
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

// keyboard
static void process_input(GLFWwindow *window);

double last_mx = 0, last_my = 0, cur_mx = 0, cur_my = 0;
int arcball_on = false;

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);
static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);


// ------- TRANSFORMATION -------
static glm::mat4 transform_shader = glm::mat4(1.0f);
glm::mat4 rotated_model;
glm::mat4 rotated_view;
glm::mat4 projection;

// ------- IMGUI -----------
void show_window(bool *p_open, GLFWwindow *window);
bool window_showed = true;
void rotation_settings();
void zoom_settings();
void set_shader();
void select_model(GLFWwindow *window);
void analyse_gaussian_curvature(GLFWwindow *window);
void initialize_texture_object(GLFWwindow *window, bool reload_mesh);

// set-up parameter imgui
static float angle = 180.0f;                // angle of rotation - must be the same of transform_shader
static float axis_x = 0.0f;                 // axis of rotation - must be the same of transform_shader
static float axis_y = 1.0f;                 // axis of rotation - must be the same of transform_shader
static float axis_z = 0.0f;                 // axis of rotation - must be the same of transform_shader
static int rotation_set = 0; // 0 mouse - 1 auto - 2 manual
static bool last_time_was_animated = false; // if was moving and now we have stopped it

static ImVec4 color_imgui = ImColor(0, 0, 0, 255);

int count_angle = 0;
bool decrease_angle = false;

// imgui shaders
static int shader_set = 3; // default 0
static int gc_set = 2;

// imgui listbox models
static int listbox_item_current = 0;
static int listbox_item_prev = 0;

void analyse_gaussian_curvature();

void set_parameters_shader(int selected_shader);

void swap_gaussian_curvature();

// ------- END IMGUI -------------

// ------ SETTINGS SHADERS ---------------
const char *vertex_shader;
const char *geometry_shader;
const char *fragment_shader;
int imgui_isGaussianCurvature;
int imgui_isExtendFlatShading;
int imgui_isGouraudShading;
int imgui_isMeanCurvatureShading;
string name_file = "models/icosahedron_1.off"; //default armadillo

float min_val, max_val;

// ----------- END SETTINGS SHADERS ----------

int main(int argc, char *argv[])
{
    set_parameters_shader(shader_set);

    /**
        ------------- GLFW -------------
        initialize glf library
     */
    glfwInit();

    // configure GLFW - OpenGl version 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for OS X
#endif

    /**
        create a window object
        glfwCreateWindow (int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
    */
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Bachelor Project", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // But on MacOS X with a retina screen it'll be 1024*2 and 768*2, so we get the actual framebuffer size:
    current_width = WIDTH;
    current_height = HEIGHT;
    glfwGetFramebufferSize(window, &current_width, &current_height);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // callback functions
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // user resizes the window the viewport should be adjusted as well
    glfwSetMouseButtonCallback(window, mouse_button_callback);         // call the callback when the user press a button. It corresponds to glutMouseFunc
    glfwSetCursorPosCallback(window, cursor_position_callback);        // call the callback when the user move the cursor. It corresponds to glutMotionFunc
    glfwSetScrollCallback(window, scroll_callback);                    //zoom

    /**
        ------------- GLAD -------------
        GLAD manages function pointers for OpenGL so we want to initialize GLAD before we call any OpenGL function
    */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // ------------- END GLAD -------------

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);

    // --------------- SHADER -------------------------------
    /**
        Modern OpenGL requires that we at least set up a vertex and fragment shader if we want to do some rendering.
        Shader language GLSL (OpenGL Shading Language)
        Each shader begins with a declaration of its version.
        Since OpenGL 3.3 and higher the version numbers of GLSL match the version of OpenGL
        (GLSL version 420 corresponds to OpenGL version 4.2 for example).
    */
    Shader ourShader = Shader();
    ourShader.setBool("custom_flag", false);

    Shader normalShader = Shader();
    normalShader.initialize_shader("normal.vs", "normal.fs", "normal.gs");

    initialize_texture_object(window, true);

    /**
        application to keep drawing images and handling user input until the program has been explicitly told to stop
        render loop
    */
    while (!glfwWindowShouldClose(window))
    { // function checks at the start of each loop iteration if GLFW has been instructed to close

        glfwPollEvents();

        // new frame imgui
        ImGui_ImplGlfwGL3_NewFrame();

        // keyboard
        process_input(window);

        glEnable(GL_DEPTH_TEST);

        // Render to our framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
        glViewport(0, 0, current_width, current_height); // Render on the whole framebuffer, complete from the lower left corner to the upper right

        // render colours
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);               //black screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the depth buffer before each render iteration (otherwise the depth information of the previous frame stays in the buffer).

        projection = glm::perspective(glm::radians(Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 10.f);

        ourShader.initialize_shader(vertex_shader, fragment_shader, geometry_shader);
        ourShader.use();

        // --- setting shaders ---
        if (imgui_isExtendFlatShading || imgui_isGouraudShading)
        {
            // get matrix's uniform location and set matrix
            ourShader.setVec3("light.position", 0.5f, 0.5f, 0.5f);
            ourShader.setVec3("viewPos", glm::vec3(0.0f, 0.0f, 3.0f));

            // light properties
            ourShader.setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
            ourShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
            ourShader.setVec3("light.specular", 0.2f, 0.2f, 0.2f);
            ourShader.setFloat("shininess", 12.0f);
        }
        else if (imgui_isGaussianCurvature)
        {
            ourShader.setFloat("min_gc", object.get_minimum_gaussian_curvature_value());
            ourShader.setFloat("max_gc", object.get_maximum_gaussian_curvature_value());
            if (gc_set == 3)
            {
                ourShader.setBool("custom_flag", true);
                ourShader.setFloat("custom_min", min_val);
                ourShader.setFloat("custom_max", max_val);
            }
            else
            {
                ourShader.setBool("custom_flag", false);
            }
        }
        // --- end settings shaders ---

        // ------ ROTATION ------
        // imgui rotation
        switch(rotation_set){
            case 1: // auto
                if (!last_time_was_animated)
                    count_angle = 0;

                angle = count_angle;

                // if(count_angle > 360){
                //     decrease_angle = true;
                // } else if(count_angle < 0){
                //     decrease_angle = false;
                // }

                // if(decrease_angle)
                //     count_angle--;
                // else
                //     count_angle++;

                if (count_angle > 360)
                    count_angle = 0;
                count_angle++;

                transform_shader = glm::rotate(glm::mat4(1.0f), glm::radians((float)angle), glm::vec3(axis_x, axis_y, axis_z));
                last_time_was_animated = true;
                ourShader.setMat4("model", transform_shader);
                ourShader.setMat4("view", view);
                ourShader.setMat4("projection", projection);
                break;

            case 2: // manual
                transform_shader = glm::rotate(glm::mat4(1.0f), glm::radians((float)angle), glm::vec3(axis_x, axis_y, axis_z));
                ourShader.setMat4("view", view);
                ourShader.setMat4("projection", projection);
                ourShader.setMat4("model", transform_shader);
                break;

            default: // mouse
                // arcball
                rotated_view = view * arcball.rotation_matrix_view();
                rotated_model = model * arcball.rotation_matrix_model(rotated_view);

                ourShader.setMat4("projection", projection);
                ourShader.setMat4("view", rotated_view);
                ourShader.setMat4("model", rotated_model);
                break;

        }

        object.draw(); // draw

        if (IS_IN_DEBUG)
        {
            // then draw model with normal visualizing geometry shader (FOR DEBUG)
            normalShader.use();
            normalShader.setMat4("projection", projection);
            normalShader.setMat4("view", rotated_view);
            normalShader.setMat4("model", rotated_model);

            object.draw(); // draw
        }

        // ---------- RENDER AS TEXTURE (FBO) --------------------

        // Render to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ---------- END RENDER AS TEXTURE (FBO) --------------------

        // ----------- IMGUI -----------------

        // Rendering
        show_window(&window_showed, window);

        ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

        // swap the buffers and check for events
        glfwSwapBuffers(window); // will swap the color buffer
        // glfwPollEvents(); // function checks if any events are triggered (like keyboard input or mouse movement events)
        glDisable(GL_DEPTH_TEST);
    }

    ourShader.deactivate();

    if (IS_IN_DEBUG)
    {
        normalShader.deactivate();
    }

    // delete the shader objects once we've linked them into the program object; we no longer need them anymore
    object.clear();
    glDeleteFramebuffers(1, &frame_buffer);
    glDeleteTextures(1, &rendered_texture);
    glDeleteRenderbuffers(1, &depth_render_buffer);

    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();

    // clean/delete all resources that were allocated
    glfwTerminate();
    return 0;
}

// whenever the window size changed this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    /**
        make sure the viewport matches the new window dimensions; note that width and
        height will be significantly larger than specified on retina displays.
    */
    current_width = width;
    current_height = height;
    glViewport(0, 0, width, height);
}

// Function that activate arcball when button left is pressed.
// mouse button, button action and modifier bits.
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    arcball.mouse_btn_callback(window, button, action, mods);
}

// Function that take position when arcball is activate (left button is pressed)
static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    arcball.cursor_position_callback(window, xpos, ypos);
}

// keyboard
static void process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (Zoom >= 1.0f && Zoom <= 45.0f)
        Zoom -= yoffset;
    if (Zoom <= 1.0f)
        Zoom = 1.0f;
    if (Zoom >= 45.0f)
        Zoom = 45.0f;
}

// IMGUI window with specific to set rotation, zoom, examples...
void show_window(bool *p_open, GLFWwindow *window)
{

    // Window for movement control
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
    static bool no_titlebar = true;
    static bool no_scrollbar = false;
    static bool no_menu = true;
    static bool no_move = true;
    static bool no_resize = false;
    static bool no_collapse = false;
    static bool no_close = true;
    static bool no_nav = false;
    static bool no_autoresize = false;

    // Demonstrate the various window flags. Typically you would just use the default.
    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)
        window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)
        window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (!no_menu)
        window_flags |= ImGuiWindowFlags_MenuBar;
    if (no_move)
        window_flags |= ImGuiWindowFlags_NoMove;
    if (no_resize)
        window_flags |= ImGuiWindowFlags_NoResize;
    if (no_collapse)
        window_flags |= ImGuiWindowFlags_NoCollapse;
    if (no_nav)
        window_flags |= ImGuiWindowFlags_NoNav;
    if (!no_autoresize)
        window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    if (no_close)
        p_open = NULL; // Don't pass our bool* to Begin

    // ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 10.0f, io.DisplaySize.y - 10.0f), ImGuiCond_Always);

    float size_x = (float)io.DisplaySize.x / 8.0f;
    // ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, color_imgui);

    ImGui::Begin("Movement control", p_open, window_flags);

    ImGui::Columns(3, "mixed");
    ImGui::Separator();
    ImGui::SetColumnOffset(0, 0);

    // ImGui::PushItemWidth(-1);

    ImGui::Text("Settings");
    if (ImGui::CollapsingHeader("Rotation"))
    {
        rotation_settings();
    }
    if (ImGui::CollapsingHeader("Zoom"))
    {
        zoom_settings();
    }
    if (ImGui::CollapsingHeader("Models"))
    {
        select_model(window);
    }
    if (ImGui::CollapsingHeader("Shaders"))
    {
        set_shader();
    }
    ImGui::NextColumn();

    ImGui::Text("ImGui"); // opengl

    ImGui::SetColumnOffset(1, size_x * 2);

    //TODO: enable arcball just in the column not in the window, otherwise when we set the sample in GC everything will be moved (same for zoom)

    // const bool itemHovered = ImGui::IsItemHovered() && ImGui::IsWindowHovered();
    // cout << ImGui::IsItemHovered(ImGuiHoveredFlags_Default) << endl;

    // --------- draw image inside GUI --------------
    //pass the texture of the FBO
    //object.getVAO() is the texture of the FBO
    //the next parameter is the upper left corner for the uvs to be applied at
    //the third parameter is the lower right corner
    //the last two parameters are the UVs
    //they have to be flipped (normally they would be (0,0);(1,1)
    double width_image = ImGui::GetCursorScreenPos().x + io.DisplaySize.x / 2;
    double height_image = ImGui::GetCursorScreenPos().y + io.DisplaySize.y / 2;

    ImGui::GetWindowDrawList()->AddImage((void *)object.getVAO(),
                                         ImVec2(ImGui::GetCursorScreenPos()),
                                         ImVec2(width_image,
                                                height_image),
                                         ImVec2(0, 1), ImVec2(1, 0));
    // ----------- end draw image inside GUI --------------------

    ImGui::NextColumn();

    ImGui::SetColumnOffset(2, size_x * 6);

    ImGui::Text("Analyse\n\n");
    if (imgui_isGaussianCurvature)
    {
        analyse_gaussian_curvature(window);
    }

    ImGui::SetCursorPosY(io.DisplaySize.y - 18.0f); // columns end at the end of window
    ImGui::NextColumn();

    // ImGui::ShowMetricsWindow(p_open); //to test and find informations about window

    ImGui::End();
    ImGui::PopStyleColor();
}

// Function to handle rotation made by GUI
void rotation_settings()
{
    ImGui::TextWrapped("You can rotate the object using the mouse, setting the angle or automatically.\n\n");
    ImGui::RadioButton("Rotate with the mouse", &rotation_set, 0);
    ImGui::RadioButton("Rotate automatically", &rotation_set, 1);
    ImGui::RadioButton("Set rotation parameters", &rotation_set, 2);

    float prev_angle, prev_axis_x, prev_axis_y, prev_axis_z;

    if (last_time_was_animated && rotation_set != 1)
    {
        axis_x = 0.0f;
        axis_y = 1.0f;
        axis_z = 0.0f;
        angle = 180;
        last_time_was_animated = false;
    }

    switch (rotation_set)
    {
        case 2:
            prev_angle = angle;
            prev_axis_x = axis_x;
            prev_axis_y = axis_y;
            prev_axis_z = axis_z;

            ImGui::Text("Set the angle of rotation:");
            ImGui::SliderFloat("angle", &angle, 0.0f, 360.0f); // Edit 1 angle from 0 to 360

            ImGui::Text("Set axis of rotation:");

            ImGui::InputFloat("x", &axis_x, 0.01f, 1.0f);
            ImGui::InputFloat("y", &axis_y, 0.01f, 1.0f);
            ImGui::InputFloat("z", &axis_z, 0.01f, 1.0f);
            break;

        default:
            break;
    }

    if ((prev_angle != angle) || (prev_axis_x != axis_x) || (prev_axis_y != axis_y) || (prev_axis_z != axis_z))
        transform_shader = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(axis_x, axis_y, axis_z));
}

// function to set zoom into imgui window
void zoom_settings()
{
    switch(rotation_set){
        case 0:
            ImGui::TextWrapped("Scroll to zoom in/out");
            break;
        default:
            ImGui::Text("Zoom-in/out:");
            ImGui::SliderFloat("zoom", &Zoom, 100, 1); // Zoom
            break;
    }

}

void set_shader()
{
    ImGui::TextWrapped("Set a shader experiment to see how the model looks like:\n\n");
    ImGui::RadioButton("Linear Interpolation", &shader_set, 0);
    ImGui::RadioButton("Extend Flat Shading", &shader_set, 1);
    ImGui::RadioButton("Gouraud Shading", &shader_set, 2);
    ImGui::RadioButton("Gaussian Curvature", &shader_set, 3);
    ImGui::RadioButton("Linear Interpolation GC", &shader_set, 4);
    ImGui::RadioButton("Mean Curvature", &shader_set, 5);

    set_parameters_shader(shader_set);
}

// function to set shader
void set_parameters_shader(int selected_shader)
{
    switch (selected_shader)
    {

    case 1: // extend flat shading
        vertex_shader = "vertexShader.vs";
        fragment_shader = "maxDiagramFragmentShader.fs";
        geometry_shader = "geometryShader.gs";

        imgui_isExtendFlatShading = 1;
        imgui_isGaussianCurvature = 0;
        imgui_isGouraudShading = 0;
        break;

        // ---------

    case 2: // gouraud shading
        vertex_shader = "vertexShader.vs";
        fragment_shader = "fragmentShader.fs";
        geometry_shader = NULL;

        imgui_isGouraudShading = 1;
        imgui_isExtendFlatShading = 0;
        imgui_isGaussianCurvature = 0;
        break;

        // ---------

    case 4: // linear interpolation Gaussian Curvature
        vertex_shader = "vertexShaderGC.vs";
        fragment_shader = "fragmentShader.fs";
        geometry_shader = NULL;

        imgui_isGaussianCurvature = 1;
        imgui_isGouraudShading = 0;
        imgui_isExtendFlatShading = 0;
        break;

    case 5: // mean curvature
        vertex_shader = "vertexShaderMC.vs";
        fragment_shader = "minDiagramFragmentShader.fs";
        geometry_shader = "geometryShaderMC.gs";

        imgui_isGaussianCurvature = 0;
        imgui_isGouraudShading = 0;
        imgui_isExtendFlatShading = 0;
        imgui_isMeanCurvatureShading = 1;
        break;

    default: // gaussian curvature (3)
        vertex_shader = "vertexShaderGC.vs";
        fragment_shader = "maxDiagramFragmentShader.fs";
        geometry_shader = "geometryShader.gs";

        imgui_isGaussianCurvature = 1;
        imgui_isExtendFlatShading = 0;
        imgui_isGouraudShading = 0;
        break;
    }
}

// function to select a model to render
void select_model(GLFWwindow *window)
{
    listbox_item_prev = listbox_item_current;
    ImGui::TextWrapped("Select a model to render:\n\n");
    const char *listbox_items[] = {"armadillo", "eight", "genus3", "horse", "icosahedron_1", "icosahedron_2", "icosahedron_3", "icosahedron_4"};
    ImGui::PushItemWidth(-1);
    ImGui::ListBox("", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 10);

    name_file = "models/" + std::string(listbox_items[listbox_item_current]) + ".off"; // generate name file

    if (listbox_item_current != listbox_item_prev)
    {
        // clean/delete all resources that were allocated
        object.clear();
        glDeleteFramebuffers(1, &frame_buffer);
        glDeleteTextures(1, &rendered_texture);
        glDeleteRenderbuffers(1, &depth_render_buffer);

        initialize_texture_object(window, true);
    }
}

/**
 * Plots about Gaussian Curvature
 */
void analyse_gaussian_curvature(GLFWwindow *window)
{
    int prev_gc = gc_set;
    ImGui::TextWrapped("Gaussian Curvature plots\n\n");
    int minimum_gc = object.get_minimum_gaussian_curvature_value();
    int maximum_gc = object.get_maximum_gaussian_curvature_value();
    ImColor green = ImColor(0, 255, 0, 255);
    ImVec4 white = ImColor(255, 255, 255, 255);
    ImVec4 red = ImColor(255, 0, 0, 255);

    static int values_offset = 0;
    static int display_count = 70;
    ImGui::SliderInt("Sample count", &display_count, 1, object.triangle_gc.size());

    // Gaussian curvature
    const char *names[] = {"GC", "ZERO"};
    const ImColor colors[2] = {green, white};

    struct Funcs
    {
        static float function(const float *data, int i)
        {
            return data[i];
        }
    };

    float (*func)(const float *data, int idx) = Funcs::function;

    const int size = object.triangle_gc.size();

    std::vector<float> zeros(size, 0.0f);

    const float **datas_initialize = new const float *[2];
    datas_initialize[0] = &object.triangle_gc[0];
    datas_initialize[1] = &zeros[0];

    const float *const *datas = datas_initialize;
    ImGui::RadioButton("Untouched GC", &gc_set, 1);
    ImGui::PlotMultiLines("##Gaussian Curvature", 2, names, colors, func, datas, display_count, minimum_gc, maximum_gc, ImVec2(0, 80));

    // automatic Gaussian Curvature
    const char *names_auto[] = {"GC", "ZERO", "MEAN"};
    const ImColor colors_auto[3] = {green, white, red};

    std::vector<float> mean(size, object.gc_helper.mean);

    const float **datas_initialize_auto = new const float *[3];
    datas_initialize_auto[0] = &object.triangle_gc_modified_auto[0];
    datas_initialize_auto[1] = &zeros[0];
    datas_initialize_auto[2] = &mean[0];

    const float *const *datas_auto = datas_initialize_auto;

    ImGui::RadioButton("Automatic best GC", &gc_set, 2);
    ImGui::PlotMultiLines("##Gaussian Curvature automatic", 3, names_auto, colors_auto, func, datas_auto, display_count, minimum_gc, maximum_gc, ImVec2(0, 80));

    // section where the user can set its own minimum and maximum value for gaussian curvature
    ImGui::RadioButton("Manual GC", &gc_set, 3);
    double min_gc, max_gc;
    if (gc_set == 2)
    { // automatic best GC
        min_gc = object.gc_helper.lower_outlier;
        max_gc = object.gc_helper.upper_outlier;
    }
    else if (gc_set == 1)
    { // classic
        min_gc = object.get_minimum_gaussian_curvature_value();
        max_gc = object.get_maximum_gaussian_curvature_value();
    }
    else
    {

        if (!min_val || !max_val)
        {
            min_gc = object.get_minimum_gaussian_curvature_value();
            max_gc = object.get_maximum_gaussian_curvature_value();
        }
        else
        {
            min_gc = min_val;
            max_gc = max_val;
        }
    }

    //  cout << min_gc << endl;
    //  cout << max_gc << endl;

    ImGui::TextWrapped("\n\nNegative values are mapped from red to green and positive values from green to blue.\n");
    ImGui::Text("min = %f, max = %f", min_gc, max_gc);
    min_val = (float)min_gc;
    max_val = (float)max_gc;
    float min_val_bound = (float)object.get_minimum_gaussian_curvature_value() * 10;
    float max_val_bound = (float)object.get_maximum_gaussian_curvature_value() * 10;
    ImGui::DragFloatRange2("range", &min_val, &max_val, 0.25f, min_val_bound, max_val_bound, "Min: %.1f", "Max: %.1f");

    if (prev_gc != gc_set)
    {
        object.clear();
        glDeleteFramebuffers(1, &frame_buffer);
        glDeleteTextures(1, &rendered_texture);
        glDeleteRenderbuffers(1, &depth_render_buffer);

        mean.clear();
        mean.shrink_to_fit();

        initialize_texture_object(window, false);
    }
}

void initialize_texture_object(GLFWwindow *window, bool reload_mesh)
{
    // --------- SET UP ------------
    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);
    // ----------------------

    /**
        NB. OpenGL works in 3D space we render a 2D triangle with each vertex having a z coordinate of 0.0.
        This way the depth of the triangle remains the same making it look like it's 2D.

        Send vertex data to vertex shader (load .off file).
     */
    object.set_value_gc(gc_set);
    // object.set_file(name_file, std::bind(&Object::auto_detect_outliers_gc, Object()), std::bind(&Object::set_selected_gc, Object()), std::bind(&Object::init, Object())); //load mesh
    if (reload_mesh)
        object.set_file(name_file); //load mesh
    object.init();

    /**
        IMPORTANT FOR TRANSFORMATION:
        Since GLM version 0.9.9, GLM default initializates matrix types to a 0-initalized matrix,
        instead of the identity matrix. From that version it is required to initialize matrix types as: glm::mat4 mat = glm::mat4(1.0f).
    */
    // camera position (eye) - look at origin - head is up
    view = glm::lookAt(glm::vec3(4.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::mat4(1.0f);
    transform_shader = glm::rotate(transform_shader, 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));

    // ---------- END SHADER -----------------

    // ---------------------------------------------
    // Render to Texture - specific code begins here
    // ---------------------------------------------

    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    frame_buffer = 0;
    glGenFramebuffers(1, &frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    // The texture we're going to render to
    glGenTextures(1, &rendered_texture);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, rendered_texture);

    // Give an empty image to OpenGL ( the last "0" means "empty" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, current_width, current_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    // Poor filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // The depth buffer
    glGenRenderbuffers(1, &depth_render_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, current_width, current_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

    // Set "rendered_texture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_texture, 0);

    // Set the list of draw buffers.
    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers); // "1" is the size of draw_buffers

    // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "error framebuffer" << endl;
        exit(-1);
    }

    // --------------- IMGUI ---------------------
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    ImGui_ImplGlfwGL3_Init(window, false);

    // Setup style
    ImGui::StyleColorsDark();
    // ---------------- END IMGUI ----------------------
}