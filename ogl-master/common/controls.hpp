#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#define WINDOWWIDTH 858
#define WINDOWHEIGHT 480

void computeMatricesFromInputs(GLFWwindow* window);
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
glm::vec3 getCameraPosition();
glm::vec3 getCameraDirection();
glm::vec3 getCameraRight();

#endif