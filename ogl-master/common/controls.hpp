#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#define WINDOWWIDTH 640
#define WINDOWHEIGHT 360

void computeMatricesFromInputs(GLFWwindow* window, bool inMenu);
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
glm::vec3 getCameraPosition();
glm::vec3 getCameraDirection();
glm::vec3 getCameraRight();

#endif