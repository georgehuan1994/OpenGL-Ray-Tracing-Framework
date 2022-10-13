//
// Created by George on 2022/9/29.
//

#ifndef TINY_GL_PATHTRACER_CAMERA_H
#define TINY_GL_PATHTRACER_CAMERA_H

#include <vector>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 30.0f;
const float FOV = 30.0f;

class Camera {
public:
    glm::mat4 ViewMatrix;
    glm::vec3 Position;
    glm::vec3 Rotation;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    float Fov;

    float ScreenRatio;
    float halfH;
    float halfW;
    glm::vec3 LeftBottomCorner;

    int LoopNum;

    Camera(float screenRatio = 1.0f,
           glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 rotation = glm::vec3(YAW, PITCH, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH) :
            ViewMatrix(glm::mat4(1)),
            Front(glm::vec3(0.0f, 0.0f, -1.0f)),
            MovementSpeed(SPEED),
            MouseSensitivity(SENSITIVITY),
            Zoom(ZOOM) {

        Position = position;
        Rotation = rotation;
        glm::mat4 rotationMat(1);
        rotationMat = glm::rotate(rotationMat, Rotation.x, glm::vec3(1.0, 0.0, 0.0));
        rotationMat = glm::rotate(rotationMat, Rotation.y, glm::vec3(0.0, 1.0, 0.0));
        Front = glm::vec3(rotationMat * glm::vec4(Front, 1.0));
        Up = glm::vec3(rotationMat * glm::vec4(Up, 1.0));
        Right = glm::vec3(rotationMat * glm::vec4(Right, 1.0));

        WorldUp = up;
        Yaw = Rotation.x;
        Pitch = Rotation.y;
        ScreenRatio = screenRatio;
        Fov = FOV;
        halfH = glm::tan(glm::radians(Zoom));
        halfW = halfH * ScreenRatio;
        LeftBottomCorner = Front - halfW * Right - halfH * Up;
        LoopNum = 0;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;

        LoopNum = 0;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = 89.0f;
        }

        Rotation = glm::vec3(Yaw, Pitch, 0.0f);
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float) yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;

        halfH = glm::tan(glm::radians(Zoom));
        halfW = halfH * ScreenRatio;
        LeftBottomCorner = Front - halfW * Right - halfH * Up;
        LoopNum = 0;
    }

    void ProcessScreenRatio(int screenWidth, int screenHeight) {
        ScreenRatio = (float) screenWidth / (float) screenHeight;
        updateCameraVectors();
    }

    void LoopIncrease() {
        LoopNum++;
    }

    void Refresh() {
        Pitch = glm::degrees(asin(Front.y));
        Yaw = glm::degrees(acos(Front.x / cos(glm::radians(Pitch)))) - 180;
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
        halfH = glm::tan(glm::radians(Zoom));
        halfW = halfH * ScreenRatio;
        LeftBottomCorner = Front - halfW * Right - halfH * Up;
        Rotation = glm::vec3(Yaw, Pitch, 0.0f);
        LoopNum = 0;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
        halfH = glm::tan(glm::radians(Zoom));
        halfW = halfH * ScreenRatio;
        LeftBottomCorner = Front - halfW * Right - halfH * Up;
        LoopNum = 0;
    }
};

#endif //TINY_GL_PATHTRACER_CAMERA_H
