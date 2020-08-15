#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#pragma once

static int WINDOWWIDTH = 640;
static int WINDOWHEIGHT = 360;

void computeMatricesFromInputs(GLFWwindow* window, bool inMenu);
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
glm::vec3 getCameraPosition();
glm::vec3 getCameraDirection();
glm::vec3 getCameraRight();
void setCameraPosition(glm::vec3 pos);
void setCameraDirection(float vDir, float hDir);

#endif