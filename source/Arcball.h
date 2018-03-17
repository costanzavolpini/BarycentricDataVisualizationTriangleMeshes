
#ifndef ARCBALL_H
#define ARCBALL_H
#include "Base.h"

class Arcball {
    int WIDTH, HEIGHT;
    float speed;
    bool x_axis, y_axis;
    glm::vec3 camera_axis = glm::vec3(0.0f, 1.0f, 0.0f);
    float angle = 0.0f;
    int event_mouse = 0;
    glm::vec3 cur_pos, prev_pos;


    public:
        // constructor
        Arcball(int w, int h, float rollSpeed, bool x, bool y){
            WIDTH = w;
            HEIGHT = h;
            x_axis = x;
            y_axis = y;
            speed = rollSpeed;
        }

        // convert coordinates of window ([0, W/H]) to normalized coordinates ([-1, 1])
        glm::vec3 get_screen_coord(double x, double y){
            glm::vec3 P(0.0f); //initialize vector

            if(x_axis)
                P.x =  (2 * x - WIDTH ) / WIDTH;
    
            if(y_axis)
                P.y = -(2 * y - HEIGHT) / HEIGHT;

            // normalized coordinates
            P.x = glm::clamp( P.x, -1.0f, 1.0f );
            P.y = glm::clamp( P.y, -1.0f, 1.0f );

            float squared_OP = P.x * P.x + P.y * P.y;
            if(squared_OP <= 1)
                P.z = sqrt(1.0 - squared_OP);
            else
                P = glm::normalize(P);
            return P;
        }

        // Function that activate arcball when button left is pressed.
        // mouse button, button action and modifier bits.
        void mouse_btn_callback(GLFWwindow * window, int button, int action, int mods){
             event_mouse = ( action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT );
        }

        // Function that take position when arcball is activate (left button is pressed)
        void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
            if(event_mouse == 0) // nothing pressed - no track
                return;
            else if(event_mouse == 1) { //first position (btn pressed)
                prev_pos = get_screen_coord(xpos, ypos);
                event_mouse = 2;
                return;
            }

            // not the first time that we have pressed the btn
            cur_pos = get_screen_coord(xpos, ypos); //get current pos
            
            // angle in radians and clip [0, 90]
            angle = acos(std::min(1.0f, glm::dot(prev_pos, cur_pos)));
            
            // cross product to get rotation axis (in camera coords)
            camera_axis = glm::cross(prev_pos, cur_pos);
        }

        // create rotation matrix with camera coords
        // multiply this matrix with view matrix to rotate the camera
        glm::mat4 create_rotation_matrix_view(){
            return glm::rotate(glm::degrees(angle) * speed, camera_axis);
        }

        // create rotation matrix with world coords
        // multiply this matrix with model matrix to rotate the object
        glm::mat4 create_rotation_matrix_model(glm::mat4& view_matrix){
            glm::vec3 axis = glm::inverse(glm::mat3(view_matrix)) * camera_axis;
            return glm::rotate(glm::degrees(angle) * speed, axis);
        }
};
#endif