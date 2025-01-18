#ifndef Camera_hpp
#define Camera_hpp

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <string>

namespace gps {
    
    enum MOVE_DIRECTION {MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT, MOVE_UP, MOVE_DOWN};
    
    class Camera {

    public:
        //Camera constructor
        Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp);
        //return the view matrix, using the glm::lookAt() function
        glm::mat4 getViewMatrix();
        //update the camera internal parameters following a camera move event
        void move(MOVE_DIRECTION direction, float speed);
        //update the camera internal parameters following a camera rotate event
        //yaw - camera rotation around the y axis
        //pitch - camera rotation around the x axis
        void rotate(float pitch, float yaw);

        //void scenePreview(float angle, glm::vec3 camPos);

        glm::vec3 getCameraPosition();
        glm::vec3 getCameraTarget();
        glm::vec3 getCameraDirection();
		glm::vec3 getCameraUpDirection();
		glm::vec3 getCameraFrontDirection();
		glm::vec3 getCameraRightDirection();

        void setCameraPosition(glm::vec3 pos);
        void setCameraDirection(glm::vec3 dir);
        void setCameraTarget(glm::vec3 target);

        //void mouse_callback(float xpos, float ypos);

    private:
        glm::vec3 cameraPosition;
        glm::vec3 cameraTarget;
        glm::vec3 cameraFrontDirection;
        glm::vec3 cameraRightDirection;
        glm::vec3 cameraUpDirection;
		glm::vec3 cameraDirection;
    };    
}

#endif /* Camera_hpp */
