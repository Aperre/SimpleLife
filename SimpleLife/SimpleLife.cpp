#include <GL/glew.h>
#include <bitset>
#include <GLFW/glfw3.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include "BS_thread_pool.hpp"
#include <string>

constexpr int WIDTH = 900;
constexpr int HEIGHT = 900;
constexpr float FRICTION = 99;
constexpr int NUM_CIRCLES = 2500;
constexpr float ATTRACTION_RADIUS = 50.0f;
constexpr float ATTRACTION_FORCE = .001f;
constexpr float RADIUS = 3.0f;
constexpr double MASS = 1.0f;

int WINDOW_WIDTH = WIDTH;
int WINDOW_HEIGHT = HEIGHT;
class Cursor {
public:
	float clickType = 0;
	float cy = 0;
	float cx = 0;
};

Cursor* cursor = new Cursor();

std::vector<float> integerToBinaryArray(int num) {
	std::vector<float> binaryArray(3, 0.0f);
	int mask = 1;

	for (int i = 0; i < 3; ++i) {
		if ((num & mask) != 0) {
			binaryArray[3 - i - 1] = 1.0f;
		}
		mask <<= 1;
	}

	return binaryArray;
}

float distance(float x1, float y1, float x2, float y2) {
	float d = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
	if (d >= ATTRACTION_RADIUS) return 10e6;
	return d;
}


class Circle {
private:
	template <typename T>
	void interactions(bool f1, T other, float interactionCoefficient) {
		float d = distance(cx, cy, other.cx, other.cy);

		if (!f1) return;

		float fx = interactionCoefficient * ATTRACTION_FORCE * (other.cx - cx) / d;
		float fy = interactionCoefficient * ATTRACTION_FORCE * (other.cy - cy) / d;

		vx += fx;
		vy += fy;
	}

public:
	float cx, cy;       // Position
	float radius;       // Radius
	float vx, vy;       // Velocity
	float red, green, blue; // Colors

	Circle(float x, float y, float ra, float vx = 0, float vy = 0, float r = 1.0, float g = 0.0, float b = 0.0)
		: cx(x), cy(y), radius(ra), vx(vx), vy(vy), red(r), green(g), blue(b) {}

	void draw(int num_segments = 100) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBegin(GL_TRIANGLE_FAN);
		glColor4f(red, green, blue, 1.0); // Solid color for the center
		glVertex2f(cx, cy); // Center of the circle
		for (int i = 0; i <= num_segments; i++) {
			float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
			float x = radius * cosf(theta);
			float y = radius * sinf(theta);
			glVertex2f(x + cx, y + cy);
		}
		glEnd();
	}

	void update(std::vector<Circle>& circles) {
		// Update position based on velocity
		cx += vx;
		cy += vy;

		// Apply friction to velocity
		vx *= 1.f - 1.f / FRICTION;
		vy *= 1.f - 1.f / FRICTION;

		// Apply force towards the center of the window
		const float CENTER_ATTRACTION_FORCE = 0.002f; // Adjust this value to control the strength of the force
		float centerX = WINDOW_WIDTH / 2.0f;
		float centerY = WINDOW_HEIGHT / 2.0f;
		float distanceFromCenterX = cx - centerX;
		float distanceFromCenterY = cy - centerY;
		float distanceFromCenter = sqrt(distanceFromCenterX * distanceFromCenterX + distanceFromCenterY * distanceFromCenterY);
		if (distanceFromCenter > 0.0f) {
			float forceX = -CENTER_ATTRACTION_FORCE * distanceFromCenterX / distanceFromCenter;
			float forceY = -CENTER_ATTRACTION_FORCE * distanceFromCenterY / distanceFromCenter;
			vx += forceX;
			vy += forceY;
		}
		if (distanceFromCenter < 200.0f) {
			float forceX = (-2 * CENTER_ATTRACTION_FORCE) * distanceFromCenterX / distanceFromCenter;
			float forceY = (-2 * CENTER_ATTRACTION_FORCE) * distanceFromCenterY / distanceFromCenter;
			vx -= forceX;
			vy -= forceY;
		}

		// Check for nearby green circles and apply attractive force
		for (Circle& other : circles) {

			if (&other == this) continue;

			float d = distance(cx, cy, other.cx, other.cy);

			if (d < ATTRACTION_RADIUS) {
				
				interactions(this->green == 1.0f, other, -1.0f);
				interactions(other.green == 1.0f, other, -0.3f);
				interactions(this->blue == 1.0f && other.red == 1.0f, other, -1.0f);
				interactions(this->red == 1.0f && other.blue == 1.0f, other, 2.0f);
				interactions(this->red == 1.0f && other.red == 1.0f, other, -0.5f);
				interactions(this->blue == 1.0f && other.blue == 1.0f, other, 2.0f);
				interactions(cursor->clickType, *cursor, cursor->clickType);


				if (d < RADIUS + RADIUS) {
					// Check circle collisions and response here
					double d = sqrt(pow(cx - other.cx, 2) + pow(cy - other.cy, 2));
					double overlap = RADIUS + RADIUS - d;

					double nx = (other.cx - cx) / d;
					double ny = (other.cy - cy) / d;

					// Separate the circles by the overlap amount
					cx -= overlap * 0.5 * nx;
					cy -= overlap * 0.5 * ny;
					other.cx += overlap * 0.5 * nx;
					other.cy += overlap * 0.5 * ny;

					double p = 2 * (vx * nx + vy * ny - other.vx * nx - other.vy * ny) / (2 * MASS);
					vx += -p * MASS * nx;
					vy += -p * MASS * ny;
					other.vx += p * MASS * nx;
					other.vy += p * MASS * ny;
				}
			}

		}

		// Handle window bounds collision (not to lose the circle from view)
		if (cx + radius < 0) {
			cx = WINDOW_WIDTH + radius;
			vx = vx + (vx) / abs(vx) * 1.25;
		}
		if (cx - radius > WINDOW_WIDTH) {
			cx = -radius;
			vx = -vx + (vx) / abs(vx) * 1.25;
		}
		if (cy + radius < 0) {
			cy = WINDOW_HEIGHT + radius;
			vy = vy + (vy) / abs(vy) * 1.25;
		}
		if (cy - 2 * radius > WINDOW_HEIGHT) {
			cy = -radius;
			vy = vy + (vy) / abs(vy) * 1.25;
		}
	}
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT && button != GLFW_MOUSE_BUTTON_RIGHT) return;
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	cursor->cx = xpos;
	cursor->cy = ypos;
	if (action == GLFW_PRESS) {
		cursor->clickType = button * 2 - 1;
	}
	else if (action == GLFW_RELEASE) {
		cursor->clickType = 0;
	}
}

int main() {
	if (!glfwInit()) {
		return -1;
	}

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Simple Life", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	

	glfwMakeContextCurrent(window);
	glewInit();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	std::vector<Circle> circles;

	// Create circles with random positions, velocities, and colors
	int SNUM_CIRCLES = sqrtf(NUM_CIRCLES);
	for (int x = 0; x < SNUM_CIRCLES; ++x) {
		for (int y = 0; y < SNUM_CIRCLES; ++y) {
			int randomColor = static_cast<float>(1 + floor(static_cast<float>(rand()) / RAND_MAX * 3));
			int color = randomColor;
			if (randomColor == 3)
				color ^= 7;
			std::vector<float> colorArray = integerToBinaryArray(color);
			float cx = static_cast<float>(x) / (SNUM_CIRCLES - 1.0) * WINDOW_WIDTH;
			float cy = static_cast<float>(y) / (SNUM_CIRCLES - 1.0) * WINDOW_HEIGHT;
			float radius = RADIUS;
			float vx = static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f; // Random velocity between -100 and 100
			float vy = static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f;
			float r = colorArray.at(0);
			float g = colorArray.at(2);
			float b = colorArray.at(1);

			circles.emplace_back(cx, cy, radius, vx, vy, r, 0, b + g);
		}
	}

	// FPS counter variables
	double lastTime = glfwGetTime();
	int frameCount = 0;

	ThreadPool pool(std::thread::hardware_concurrency());

	while (!glfwWindowShouldClose(window)) {
		glfwGetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (cursor->clickType) {
			double x;
			double y;
			glfwGetCursorPos(window, &x, &y);
			cursor->cx = x;
			cursor->cy = y;
		}

		double currentTime = glfwGetTime();
		frameCount++;
		if (currentTime - lastTime >= 1.0) { // If a second has passed
			double fps = double(frameCount) / (currentTime - lastTime);
			std::string title = "SimpleLife - FPS: " + std::to_string(fps);
			glfwSetWindowTitle(window, title.c_str());
			frameCount = 0;
			lastTime = currentTime;
		}

		// Enqueue update tasks for each circle
		for (Circle& circle : circles) {
			pool.enqueue([&circle, &circles]() {
				circle.update(circles);
				});
		}

		// Wait for all update tasks to complete
		pool.wait();

		// Now draw all circles
		for (Circle& circle : circles) {
			circle.draw();
		}

		glfwSwapBuffers(window);
		glfwPollEvents();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
