#include "Camera.hpp"

namespace gps {

    // Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        // Initialize camera position, target, and up direction
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        // Calculate the camera direction vector
        this->cameraDirection = glm::normalize(cameraTarget - cameraPosition);
      }

    // Return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    // Update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        // Calculate the front and right direction vectors
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraUpDirection, this->cameraFrontDirection));

        // Update the camera position based on the direction and speed
        switch (direction) {
        case MOVE_FORWARD:
            cameraPosition += cameraFrontDirection * speed;
            break;
        case MOVE_BACKWARD:
            cameraPosition -= cameraFrontDirection * speed;
            break;
        case MOVE_RIGHT:
            cameraPosition += cameraRightDirection * speed;
            break;
        case MOVE_LEFT:
            cameraPosition -= cameraRightDirection * speed;
            break;
		case MOVE_UP:
			cameraPosition += cameraUpDirection * speed;
            break;
		case MOVE_DOWN:
			cameraPosition -= cameraUpDirection * speed;
			break;
        }

        // Update the camera target position
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    // Update the camera internal parameters following a camera rotate event
    // yaw - camera rotation around the y axis
    // pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        // Create a rotation matrix and apply yaw and pitch rotations
        glm::mat4 matrix = glm::mat4(1.0f);
        matrix = glm::rotate(matrix, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        matrix = glm::rotate(matrix, glm::radians(pitch), glm::normalize(glm::cross(cameraUpDirection, cameraTarget - cameraPosition)));
        // Update the camera target position based on the rotation
        cameraTarget = cameraPosition + glm::vec3(matrix * glm::vec4(cameraTarget - cameraPosition, 1.0f));
    }

    // Get the current camera position
	glm::vec3 Camera::getCameraPosition() {
		return this->cameraPosition;
	}

    // Get the current camera target
	glm::vec3 Camera::getCameraTarget() {
		return this->cameraTarget;
	}

    // Get the current camera direction
    glm::vec3 Camera::getCameraDirection()
    {
        return cameraDirection;
    }

	// Get the current camera up direction
	glm::vec3 Camera::getCameraUpDirection()
	{
		return cameraUpDirection;
	}

	// Get the current camera front direction
	glm::vec3 Camera::getCameraFrontDirection()
	{
		return cameraFrontDirection;
	}

	// Get the current camera right direction
	glm::vec3 Camera::getCameraRightDirection()
	{
		return cameraRightDirection;
	}

    void Camera::setCameraPosition(glm::vec3 pos)
    {
        cameraPosition = pos;
    }

    void Camera::setCameraDirection(glm::vec3 dir)
    {
        cameraDirection = dir;
    }

    void Camera::setCameraTarget(glm::vec3 target) {
        cameraTarget = target;
    }

    //void Camera::scenePreview(float angle, glm::vec3 camPos) {
    //    // set the camera
    //    this->cameraPosition = camPos;

    //    // rotate with specific angle around Y axis
    //    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));

    //    // compute the new position of the camera 
    //    // previous position * rotation matrix
    //    this->cameraPosition = glm::vec4(rotation * glm::vec4(this->cameraPosition, 1));
    //    this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
    //    cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    //}
}
