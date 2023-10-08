#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <chrono>
// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};



std::vector<Model> models;
// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 5.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    BoundingBox bb;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    bool isJumping = false; // Is the camera currently jumping?
     float JUMP_FORCE = 3.0f; // The force of the jump
    glm::vec3 StartPosition;
    std::chrono::steady_clock::time_point FallStartTime;
    bool isFalling = false;

    void Jump(bool gravity)
    {
        if (gravity) {
            if (!isJumping) // If the camera is not already jumping
            {
                isJumping = true; // Start jumping
                Jump(gravity);
                // Apply the jump force
            }
            if (isJumping) {
                Position.y += 0.1f;
                JUMP_FORCE -= 0.1f;
                if (JUMP_FORCE <= 0) {
                    JUMP_FORCE = 3.0f;
                    isJumping = false;
                }
                else {
                    Jump(gravity);
                }
            }
        }
    }
    const float GRAVITY = -9.8f; // Gravitational constant

    void ApplyGravity(float deltaTime, bool gravity)
    {
        if (gravity) {
            glm::vec3 newPosition = Position;
            newPosition.y += GRAVITY * deltaTime; // Apply gravity
            if (!isCollision(newPosition)) // If the new position does not collide with the floor or another model
            {
                Position = newPosition; // Update position
                if (!isFalling) // If the camera was not falling before
                {
                    isFalling = true; // Start falling
                    StartPosition = Position; // Remember the starting position
                    FallStartTime = std::chrono::steady_clock::now(); // Remember the starting time
                }
                else // If the camera was already falling
                {
                    // Check if the fall duration exceeds 4 seconds
                    auto now = std::chrono::steady_clock::now();
                    auto fallDuration = std::chrono::duration_cast<std::chrono::seconds>(now - FallStartTime);
                    if (fallDuration.count() > 4)
                    {
                        Position = StartPosition; // Reset the position
                        isFalling = false; // Stop falling
                    }
                }
            }
            else if (isFalling) // If the camera collides with the floor or another model and was falling before
            {
                isFalling = false; // Stop falling
            }
        }
    }
    void updateBoundingBox(glm::vec3 newPosition) {
        bb.min = newPosition - glm::vec3(0.9f, 0.9f, 0.9f);  // No offset below the camera's position
        bb.max = newPosition + glm::vec3(1.0f, 1.0f, 1.0f);  // Offset above the camera's position
    }
   

    bool checkCollision(const BoundingBox& bb1, const BoundingBox& bb2) {
        // Check if bb1's max is greater than bb2's min and bb1's min is less than bb2's max
        return (bb1.max.x >= bb2.min.x && bb1.min.x <= bb2.max.x) &&
            (bb1.max.y >= bb2.min.y && bb1.min.y <= bb2.max.y) &&
            (bb1.max.z >= bb2.min.z && bb1.min.z <= bb2.max.z);
    }
    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        glm::vec3 newPosition;

        if (direction == FORWARD) {
            newPosition = Position + Front * velocity;
            if (!isCollision(newPosition))
                Position += Front * velocity;
        }
        if (direction == BACKWARD) {
            newPosition = Position - Front * velocity;
            if (!isCollision(newPosition))
                Position -= Front * velocity;
        }
        if (direction == LEFT) {
            newPosition = Position - Right * velocity;
            if (!isCollision(newPosition))
                Position -= Right * velocity;
        }
        if (direction == RIGHT) {
            newPosition = Position + Right * velocity;
            if (!isCollision(newPosition))
                Position += Right * velocity;
        }
        
    }
    bool isCollision(glm::vec3 position)
    {
        updateBoundingBox(position);

        for (auto& model : models)
        {
            if (checkCollision(bb, model.calculateBoundingBox()))
            {
                isJumping = false; // The camera has landed
                return true;
            }
        }

        return false;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
#endif#pragma once
