/* Marisa Kuyava
* CS 330

* 
* Controlls:
* 
* Keyboard:
*	F: Resets View
*	O: Toggle to Orthographic View 
*   P: Toggle to Persepctive View
*   W: Forward
*	A: Backward
*   S: Left
*	D: Right
*	Q: Up
*	E: Down
* 
* Mouse:
*	Zoom (Mouse Wheele)
*	Pan (Alt + MMB + Move)
*	Orbit (ALT + LMB + Move)
*/

#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Include SOIL2 header file
#include <SOIL2/SOIL2.h>


using namespace std;

int width, height;
const double PI = 3.14159;
const float toRadians = PI / 180.0f;
bool ortho = false;

// Declare Input Callback Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mode);

// Declare Matrix
glm::mat4 viewMatrix(1.0f), mvMatrix(1.0f), projectionMatrix(1.0f), modelMatrix(1.0f);

// Camera Field of View
GLfloat fov = 45.0f;

void initiateCamera();

// Define Camera Attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 6.0f); // Move 6 units back in z towards screen
glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f); // What the camera points to
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target); // direction z
glm::vec3 worldUp = glm::vec3(0.0, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));// right vector x
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight)); // up vector y
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // 1 unit away from lense
GLfloat cameraSpeed = 0.05f;

// Camera Transformation Prototype
void TransformCamera();

// Boolean array for keys and mouse buttons
bool keys[1024], mouseButtons[3];

// Input state booleans
bool isPanning = false, isOrbiting = false;

// Pitch and Yaw
GLfloat radius = 3.0f, rawYaw = 0.0f, rawPitch = 0.0f, degYaw, degPitch;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLfloat lastX = 320, lastY = 240, xChange, yChange; // Center mouse cursor
bool firstMouseMove = true;

//Light source position
glm::vec3 lightPosition(1.5f, 2.5f, 0.0f);
glm::vec3 lightTwoPosition(-1.5f, 2.5f, 0.0f);

// Draw Primitive(s)
void draw()
{
	GLenum mode = GL_TRIANGLES;
	GLsizei indices = 18;
	glDrawElements(mode, indices, GL_UNSIGNED_BYTE, nullptr);
}

/* GLSL Error Checking Definitions */
void PrintShaderCompileError(GLuint shader)
{
	int len = 0;
	int chWritten = 0;
	char* log;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	if (len > 0)
	{
		log = (char*)malloc(len);
		glGetShaderInfoLog(shader, len, &chWritten, log);
		cout << "Shader Compile Error: " << log << endl;
		free(log);
	}
}

void PrintShaderLinkingError(int prog)
{
	int len = 0;
	int chWritten = 0;
	char* log;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 0)
	{
		log = (char*)malloc(len);
		glGetShaderInfoLog(prog, len, &chWritten, log);
		cout << "Shader Linking Error: " << log << endl;
		free(log);
	}
}

bool IsOpenGLError()
{
	bool foundError = false;
	int glErr = glGetError();
	while (glErr != GL_NO_ERROR)
	{
		cout << "glError: " << glErr << endl;
		foundError = true;
		glErr = glGetError();
	}
	return foundError;
}
/* GLSL Error Checking Definitions End Here */


// Create and Compile Shaders
static GLuint CompileShader(const string& source, GLuint shaderType)
{
	// Create Shader object
	GLuint shaderID = glCreateShader(shaderType);
	const char* src = source.c_str();

	// Attach source code to Shader object
	glShaderSource(shaderID, 1, &src, nullptr);

	// Compile Shader
	glCompileShader(shaderID);

	// Return ID of Compiled shader
	return shaderID;
}

// Create Program Object
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader)
{
	// Compile vertex shader
	GLuint vertexShaderComp = CompileShader(vertexShader, GL_VERTEX_SHADER);

	// Compile fragment shader
	GLuint fragmentShaderComp = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	// Create program object
	GLuint shaderProgram = glCreateProgram();

	// Attach vertex and fragment shaders to program object
	glAttachShader(shaderProgram, vertexShaderComp);
	glAttachShader(shaderProgram, fragmentShaderComp);

	// Link shaders to create executable
	glLinkProgram(shaderProgram);

	// Delete compiled vertex and fragment shaders
	glDeleteShader(vertexShaderComp);
	glDeleteShader(fragmentShaderComp);

	// Return Shader Program
	return shaderProgram;
}

//Define vertex data for lamp
	GLfloat lampVertices[] = {
		// Triangle 1
		-0.5, -0.5, 0.0, // index 0
		-0.5, 0.5, 0.0, // index 1
		0.5, -0.5, 0.0,  // index 2	
		0.5, 0.5, 0.0  // index 3	
	};

	//Define vertex data for Cube and Table
	GLfloat vertices[] = {

		// Triangle 1
		-0.5, -0.5, 0.0, // index 0
		1.0, 0.0, 0.0, // red
		0.0, 0.0, //UV Bottom Left
		0.0f, 0.0f, 1.0f, //Normal positive z

		-0.5, 0.5, 0.0, // index 1
		0.0, 1.0, 0.0, // green
		0.0, 1.0, //UV Top Left
		0.0f, 0.0f, 1.0f, //Normal positive z

		0.5, -0.5, 0.0,  // index 2	
		0.0, 0.0, 1.0, // blue
		1.0, 0.0, //UV Bottom Right 
		0.0f, 0.0f, 1.0f, //Normal positive z

		// Triangle 2	
		0.5, 0.5, 0.0,  // index 3	
		1.0, 0.0, 1.0, // purple
		1.0, 1.0, //UV Top Right 
		0.0f, 0.0f, 1.0f, //Normal positive z
	};

	//Cube and Table
	GLubyte indices[] = {
		0, 1, 2,
		1, 2, 3
	};

	// Plane Transforms
	glm::vec3 planePositions[] = {
		glm::vec3(0.0f,  0.0f,  0.5f),
		glm::vec3(0.5f,  0.0f,  0.0f),
		glm::vec3(0.0f,  0.0f,  -0.5f),
		glm::vec3(-0.5f, 0.0f,  0.0f),
		glm::vec3(0.0f, 0.5f,  0.0f),
		glm::vec3(0.0f, -0.5f,  0.0f)
	};

	glm::float32 planeRotations[] = {
		0.0f, 90.0f, 180.0f, -90.0f, -90.0f, 90.0f
	};

	// Define vertex data for Pencil tip
	GLfloat triangleVertices[] = {

		//Pyramid - Pencil tip
		0.0f, 0.0f, 2.0f,  // vert 1
		1.8f, 0.0f, 1.0f, // pink
		0.0, 1.0, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

		0.5f, 0.866f, 0.0f, // vert 2
		0.8f, 1.0f, 0.8f, // white
		0.0, 0.0, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

		1.0f, 0.0f, 0.0f, // vert 3
		0.8f, 1.0f, 0.8f, // white
		0.0, 0.0, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

	};

	// Define vertex data for Pencil body
	GLfloat cylinderVertices[] = {

		//Cylinder - Pencil body
		0.5f, 0.866f, 0.0f, // vert 1
		1.0f, 0.0f, 1.0f, // pink
		1.0, 0.0, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.5f, 0.866f, -40.0f, // vert 2
			1.0f, 0.0f, 1.0f, // pink	
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				1.0f, 0.0f, 0.0f, // vert 3
				1.0f, 0.0f, .5f, // pink
				0.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		1.0f, 0.0f, 0.0f, // vert 4
		1.0f, 0.0f, 1.0f, // pink
		0.0, 0.0, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			1.0f, 0.0f, -40.0f, // vert 5
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z
		
				0.5f, 0.866f, -40.0f, // vert 6
				1.0f, 0.0f, 1.0f, // pink
				0.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		0.0f, .0f, -40.0f,  // vert 10
		1.0f, 0.0f, 0.0f, // red
		0.0, 0.0, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.5f, 0.866f, -40.0f, // vert 11
			1.0f, 1.0f, 1.0f, // green
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				1.0f, 0.0f, -40.0f, // vert 12
				0.0f, 0.0f, 1.0f, // blue
				0.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z
	};

	// Array for triangle rotations
	glm::float32 triRotations[] = { 0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f };


	// Define vertex data for Silver ball
	GLfloat sphereVertices[] = {
		// Top - Single
		0.0f, 0.0f, 1.0f,  // vert 1
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.43301f, -0.25f, 0.86603f, // vert 2
			1.0f, 0.0f, 1.0f, // pink	
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.43301f, 0.25f, 0.86603f, // vert 3
				1.0f, 0.0f, .5f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Row One - Tringle One
		0.43301f, -0.25f, 0.86603f, // vert 2
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.43301f, 0.25f, 0.86603f, // vert 3
			1.0f, 0.0f, 1.0f, // pink
			0.5f, 1.0f, //UV
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.75f, -0.43301f, 0.5f, // vert 4
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Row One - Tringle Two
		0.43301f, 0.25f, 0.86603f, // vert 3
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.75f, -0.43301f, 0.5f, // vert 4
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.75f, 0.43301f, 0.5f, // vert 5
				1.0f, 0.0f, 0.0f, // red
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z
		
		//Row Two - Tringle One
		0.75f, -0.43301f, 0.5f, // vert 4
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.75f, 0.43301f, 0.5f, // vert 5
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.86603f, -0.5f, 0.0f, // vert 6
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Row Two - Tringle Two
		0.75f, 0.43301f, 0.5f, // vert 5
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.86603f, -0.5f, 0.0f, // vert 6
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.86603f, 0.5f, 0.0f, // vert 7
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Row Three - Tringle One
		0.86603f, -0.5f, 0.0f, // vert 6
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.86603f, 0.5f, 0.0f, // vert 7
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.75f, -0.43301f, -0.5f, // vert 8
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Row Three - Tringle Two
		0.86603f, 0.5f, 0.0f, // vert 7
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.75f, -0.43301f, -0.5f, // vert 8
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.75f, 0.43301f, -0.5f, // vert 9
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Row Four - Tringle One
		0.75f, -0.43301f, -0.5f, // vert 8
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z
			
			0.75f, 0.43301f, -0.5f, // vert 9
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.43301f, -0.25f, -0.86603f, // vert 10
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Row Four - Tringle One
		0.75f, 0.43301f, -0.5f, // vert 9
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.43301f, -0.25f, -0.86603f, // vert 10
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.43301f, 0.25f, -0.86603f, // vert 11
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//Bottom
		0.43301f, -0.25f, -0.86603f, // vert 10
		1.0f, 0.0f, 1.0f, // pink
		0.5f, 1.0f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.43301f, 0.25f, -0.86603f, // vert 11
			1.0f, 0.0f, 1.0f, // pink
			0.0, 0.0, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.0f, 0.0f, -1.0f, // vert 12
				1.0f, 0.0f, 1.0f, // pink
				1.0, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z
	};

	// Define vertex data for Ring
	GLfloat torusVertices[] = 
	{	
		// Top - One
		0.875f, 0.0f, .21651f,  // vert 1
		1.0f, 0.0f, 1.0f, // pink
		0.875f, 0.0f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.75777f, 0.4375f, .21651f, // vert 2
			1.0f, 0.0f, 1.0f, // pink	
			0.75777, 0.4375, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				1.125f, 0.0f, 0.21651f, // vert 3
				1.0f, 0.0f, .5f, // pink
				1.125, 0.0, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		// Top - Two
		0.75777f, 0.4375f, .21651f, // vert 2
		1.0f, 0.0f, 1.0f, // pink
		0.75777f, 0.4375f, //UV
		0.0f, 0.0f, 1.0f, //Normal positive z

			1.125f, 0.0f, 0.21651f, // vert 3
			1.0f, 0.0f, 1.0f, // pink
			1.125f, 0.0f, //UV
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.97428f, 0.5625f, 0.21651f, // vert 4
				1.0f, 0.0f, 1.0f, // pink
				0.97428f, 0.5625f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		// TopSide - One
		1.125f, 0.0f, 0.21651f, // vert 3
		1.0f, 0.0f, 1.0f, // pink
		1.125f, 0.0f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.97428f, 0.5625f, 0.21651f, // vert 4
			1.0f, 0.0f, 1.0f, // pink	
			0.97428f, 0.5625f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				1.25f, 0.0f, 0.0f, // vert 5
				1.0f, 0.0f, .5f, // pink
				1.25f, 0.0f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

	
		// TopSide - Two
		0.97428f, 0.5625f, 0.21651f, // vert 4
		1.0f, 0.0f, 1.0f, // pink
		0.97428f, 0.5625f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			1.25f, 0.0f, 0.0f, // vert 5
			1.0f, 0.0f, 1.0f, // pink	
			1.25f, 0.0f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				1.0825f, 0.625f, 0.0f, // vert 6
				1.0f, 0.0f, .5f, // pink
				1.0825f, 0.625f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		// BottomSide - One
		1.25f, 0.0f, 0.0f, // vert 5
		1.0f, 0.0f, 1.0f, // pink	
		1.25f, 0.0f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z
			
			1.0825f, 0.625f, 0.0f, // vert 6
			1.0f, 0.0f, .5f, // pink
			1.0825f, 0.625f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z
				
				1.125f, 0.0f, -0.21651f, // vert 7
				1.0f, 0.0f, 1.0f, // pink	
				1.125f, 0.0f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		// BottomSide - Two
		1.0825f, 0.625f, 0.0f, // vert 6
		1.0f, 0.0f, .5f, // pink
		1.0825f, 0.625f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

				1.125f, 0.0f, -0.21651f, // vert 7
				1.0f, 0.0f, 1.0f, // pink	
				1.125f, 0.0f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z
			
					0.97428f, 0.5625f, -0.21651f, // vert 8
					1.0f, 0.0f, 1.0f, // pink	
					0.97428f, 0.5625f, //UV 
					0.0f, 0.0f, 1.0f, //Normal positive z

		// Bottom - One
		1.125f, 0.0f, -0.21651f, // vert 7
		1.0f, 0.0f, 1.0f, // pink	
		1.125f, 0.0f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.97428f, 0.5625f, -0.21651f, // vert 8
			1.0f, 0.0f, 1.0f, // pink	
			0.97428f, 0.5625f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.875f, 0.0f, -0.21651f,  // vert 9
				1.0f, 0.0f, 1.0f, // pink
				0.875f, 0.0f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z


		// Bottom - Two
		0.97428f, 0.5625f, -0.21651f, // vert 8
		1.0f, 0.0f, 1.0f, // pink	
		0.0, 0.0, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.875f, 0.0f, -0.21651f,  // vert 9
			1.0f, 0.0f, 1.0f, // pink
			0.5f, 1.0f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.75777f, 0.4375f, -0.21651f,  // vert 10
				1.0f, 0.0f, 1.0f, // pink
				0.5f, 1.0f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		// BottomInSide - One
		0.875f, 0.0f, -0.21651f,  // vert 9
		1.0f, 0.0f, 1.0f, // pink
		0.875f, 0.0f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.75777f, 0.4375f, -0.21651f,  // vert 10
			1.0f, 0.0f, 1.0f, // pink
			0.75777f, 0.4375f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.75f, 0.0f, 0.0f,  // vert 11
				1.0f, 0.0f, 1.0f, // pink
				0.75f, 0.0f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z
		
		// BottomInSide - Two
		0.75777f, 0.4375f, -0.21651f,  // vert 10
		1.0f, 0.0f, 1.0f, // pink
		0.75777f, 0.4375f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.75f, 0.0f, 0.0f,  // vert 11
			1.0f, 0.0f, 1.0f, // pink
			0.75f, 0.0f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.64952f, 0.375f, 0.0f,  // vert 12
				1.0f, 0.0f, 1.0f, // pink
				0.64952f, 0.375f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z

		//TopInSide - One
		0.75f, 0.0f, 0.0f,  // vert 11
		1.0f, 0.0f, 1.0f, // pink
		0.75f, 0.0f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z
			
			0.64952f, 0.375f, 0.0f,  // vert 12
			1.0f, 0.0f, 1.0f, // pink
			0.64952f, 0.375f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

				0.875f, 0.0f, .21651f,  // vert 1
				1.0f, 0.0f, 1.0f, // pink
				0.875f, 0.0f, //UV 
				0.0f, 0.0f, 1.0f, //Normal positive z
		
		//TopInSide - Two
		0.64952f, 0.375f, 0.0f,  // vert 12
		1.0f, 0.0f, 1.0f, // pink
		0.64952f, 0.375f, //UV 
		0.0f, 0.0f, 1.0f, //Normal positive z

			0.875f, 0.0f, .21651f,  // vert 1
			1.0f, 0.0f, 1.0f, // pink
			0.875f, 0.0f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z

			0.75777f, 0.4375f, .21651f, // vert 2
			1.0f, 0.0f, 1.0f, // pink	
			0.75777f, 0.4375f, //UV 
			0.0f, 0.0f, 1.0f, //Normal positive z
	};

	glm::float32 torusRotations[] = { 0.0f, 30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 180.0f, 210.0f, 240.0f, 270.0f, 300.0f, 330.0f };


int main(void)
{
	width = 640; height = 480;

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "Main Window", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Set input callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
		cout << "Error!" << endl;

	
	// Setup some OpenGL options
	glEnable(GL_DEPTH_TEST);

	/*TEXTURES*/
	//Load textures
	int marioTexWidth, marioTexHeight, penciltipTexWidth, penciltipTexHeight, pencilBodyTexWidth, pencilBodyTexHeight, tableTexWidth, tableTexHeight, silverTexWidth, silverTexHeight, pinkStripeTexWidth, pinkStripeTexHeight;
	unsigned char* marioImage = SOIL_load_image("mario.png", &marioTexWidth, &marioTexHeight, 0, SOIL_LOAD_RGB);
	unsigned char* pencilTipImage = SOIL_load_image("penciltip.png", &penciltipTexWidth, &penciltipTexHeight, 0, SOIL_LOAD_RGB);
	unsigned char* pencilBodyImage = SOIL_load_image("pink.png", &pencilBodyTexWidth, &pencilBodyTexHeight, 0, SOIL_LOAD_RGB);
	unsigned char* tableImage = SOIL_load_image("table.png", &tableTexWidth, &tableTexHeight, 0, SOIL_LOAD_RGB);
	unsigned char* silverImage = SOIL_load_image("silver.png", &silverTexWidth, &silverTexHeight, 0, SOIL_LOAD_RGB);
	unsigned char* pinkStripeImage = SOIL_load_image("pinkStripe.png", &pinkStripeTexWidth, &pinkStripeTexHeight, 0, SOIL_LOAD_RGB);

	//Generate Textures
	GLuint marioTexture;
	glGenTextures(1, &marioTexture);
	glBindTexture(GL_TEXTURE_2D, marioTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, marioTexWidth, marioTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, marioImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(marioImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint pencilTipTexture;
	glGenTextures(1, &pencilTipTexture);
	glBindTexture(GL_TEXTURE_2D, pencilTipTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, penciltipTexWidth, penciltipTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pencilTipImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(pencilTipImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint pencilBodyTexture;
	glGenTextures(1, &pencilBodyTexture);
	glBindTexture(GL_TEXTURE_2D, pencilBodyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pencilBodyTexWidth, pencilBodyTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pencilBodyImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(pencilBodyImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint tableTexture;
	glGenTextures(1, &tableTexture);
	glBindTexture(GL_TEXTURE_2D, tableTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tableTexWidth, tableTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, tableImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(tableImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint silverTexture;
	glGenTextures(1, &silverTexture);
	glBindTexture(GL_TEXTURE_2D, silverTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, silverTexWidth, silverTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, silverImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(silverImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint pinkStripeTexture;
	glGenTextures(1, &pinkStripeTexture);
	glBindTexture(GL_TEXTURE_2D, pinkStripeTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pinkStripeTexWidth, pinkStripeTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pinkStripeImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(pinkStripeImage);
	glBindTexture(GL_TEXTURE_2D, 0);
	/*END TEXTURES*/

	// Wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	/* Create VBO, EBO and VAOs*/
	GLuint cubeVBO, cubeEBO, cubeVAO, tableVBO, tableEBO, tableVAO, triangleVBO, triangleEBO, triangleVAO, cylinderVBO, cylinderEBO, cylinderVAO, sphereVBO, sphereEBO, sphereVAO, torusVBO, torusEBO, torusVAO, lampVBO, lampEBO, lampVAO;
	
	// Create VBO, EBO and VAO

	//Cube
	glGenBuffers(1, &cubeVBO); 
	glGenBuffers(1, &cubeEBO); 
	glGenVertexArrays(1, &cubeVAO);

	//Table
	glGenBuffers(1, &tableVBO); 
	glGenBuffers(1, &tableEBO); 
	glGenVertexArrays(1, &tableVAO);

	//Pencil Tip
	glGenBuffers(1, &triangleVBO); 
	glGenBuffers(1, &triangleEBO); 
	glGenVertexArrays(1, &triangleVAO);
	
	//Pencil Base
	glGenBuffers(1, &cylinderVBO); 
	glGenBuffers(1, &cylinderEBO); 
	glGenVertexArrays(1, &cylinderVAO);

	//Ball
	glGenBuffers(1, &sphereVBO); 
	glGenBuffers(1, &sphereEBO); 
	glGenVertexArrays(1, &sphereVAO);

	//Ring
	glGenBuffers(1, &torusVBO); 
	glGenBuffers(1, &torusEBO); 
	glGenVertexArrays(1, &torusVAO);

	//Lamp
	glGenBuffers(1, &lampVBO); 
	glGenBuffers(1, &lampEBO); 
	glGenVertexArrays(1, &lampVAO);

	glBindVertexArray(cubeVAO);

			// VBO and EBO Placed in User-Defined VAO
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO); // Select VBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO); // Select EBO

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Load vertex attributes
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

			// Specify attribute location and layout to GPU
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0); //vertex
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); //color
			glEnableVertexAttribArray(1);
			
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat))); //UV
			glEnableVertexAttribArray(2);
		
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat))); 	//Normals
			glEnableVertexAttribArray(3);

	glBindVertexArray(0); // Unbind VOA 


	glBindVertexArray(tableVAO);

			glBindBuffer(GL_ARRAY_BUFFER, tableVBO); // Select VBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tableEBO); // Select EBO

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Load vertex attributes
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

			// Specify attribute location and layout to GPU
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0); //vertex
			glEnableVertexAttribArray(0);
					
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); 	//color
			glEnableVertexAttribArray(1);
						
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat))); //UV
			glEnableVertexAttribArray(2);
					
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat))); 	//Normals
			glEnableVertexAttribArray(3);

	glBindVertexArray(0);// Unbind VOA 


	glBindVertexArray(triangleVAO);

			glBindBuffer(GL_ARRAY_BUFFER, triangleVBO); // Select VBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO); // Select EBO

			glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW); // Load vertex attributes
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

			// Specify attribute location and layout to GPU
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0); //vertex
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); //color
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat))); //UV
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat))); //Normals
			glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	glBindVertexArray(cylinderVAO);

			glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO); // Select VBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinderEBO); // Select EBO

			glBufferData(GL_ARRAY_BUFFER, sizeof(cylinderVertices), cylinderVertices, GL_STATIC_DRAW); // Load vertex attributes
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 
			
			// Specify attribute location and layout to GPU
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0); //vertex
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); //color
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat))); 	//UV
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));//Normals
			glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	glBindVertexArray(sphereVAO);

		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO); // Select EBO

		glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertices), sphereVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0); //vertex
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); //color
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat))); //UV
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));//Normals
		glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	
	glBindVertexArray(torusVAO);

		glBindBuffer(GL_ARRAY_BUFFER, torusVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torusEBO); // Select EBO

		glBufferData(GL_ARRAY_BUFFER, sizeof(torusVertices), torusVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0); //vertex
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); //color
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat))); //UV
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));//Normals
		glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	//Define lampVAO
	glBindVertexArray(lampVAO);

			glBindBuffer(GL_ARRAY_BUFFER, lampVBO); // Select VBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO); // Select EBO

			glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW); // Load vertex attributes
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0); //vertex
			glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	/* SHADERS */
	// Vertex shader source code
	string vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"layout(location = 1) in vec3 aColor;"
		"layout(location = 2) in vec2 texCoord;"
		"layout(location = 3) in vec3 normal;"
		"out vec3 oColor;"
		"out vec2 oTexCoord;"
		"out vec3 oNormal;"
		"out vec3 FragPos;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"oColor = aColor;"
		"oTexCoord = texCoord;"
		"oNormal = mat3(transpose(inverse(model))) * normal;"
		"FragPos = vec3(model * vec4(vPosition, 1.0f));"
		"}\n";

	// Fragment shader source code
	string fragmentShaderSource =
		"#version 330 core\n"
		"in vec3 oColor;"
		"in vec2 oTexCoord;"
		"in vec3 oNormal;"
		"in vec3 FragPos;"
		"out vec4 fragColor;"
		"uniform sampler2D myTexture;"
		"uniform vec3 objectColor;"
		"uniform vec3 lightColor;"
		"uniform vec3 lightPos;"
		"uniform vec3 lightTwoColor;"
		"uniform vec3 lightTwoPos;"
		"uniform vec3 viewPos;"
		"void main()\n"
		"{\n"
		"//Ambient(comment)\n"
		"float ambientStrength = 1.0f;"
		"vec3 ambient = ambientStrength * lightColor;"
		"float ambientTwoStrength = 0.0f;"
		"vec3 ambientTwo = ambientTwoStrength * lightTwoColor;"
		"//Diffuse(commnect)\n"
		"vec3 norm = normalize(oNormal);"
		"vec3 lightDir = normalize(lightPos - FragPos);"
		"float diff = max(dot(norm, lightDir), 0.5);"
		"vec3 diffuse = diff * lightColor;"
		"vec3 normTwo = normalize(oNormal);"
		"vec3 lightTwoDir = normalize(lightTwoPos - FragPos);"
		"float diffTwo = max(dot(norm, lightTwoDir), 0.1);"
		"vec3 diffuseTwo = diffTwo * lightTwoColor;"
		"//Specularity(comment)\n"
		"float specularStrength = 1.0f;"
		"vec3 viewDir = normalize(viewPos - FragPos);"
		"vec3 relfectDir = reflect(-lightDir, norm);"
		"float spec = pow(max(dot(viewDir, relfectDir), 0.0), 256);"
		"vec3 specular = specularStrength * spec * lightColor;"
		"float specularTwoStrength = 0.8f;"
		"vec3 viewDirTwo = normalize(viewPos - FragPos);"
		"vec3 relfectDirTwo = reflect(-lightTwoDir, norm);"
		"float specTwo = pow(max(dot(viewDirTwo, relfectDirTwo), 0.0), 256);"
		"vec3 specularTwo = specularTwoStrength * specTwo * lightTwoColor;"
		"vec3 result = ((ambient + diffuse + specular) + (ambientTwo + diffuseTwo + specularTwo)) * objectColor;"
		"fragColor = texture(myTexture, oTexCoord) * vec4(result, 1.0f);"
		"}\n";

	// Lamp Vertex shader source code
	string lampVertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"}\n";

	// Lamp Fragment shader source code
	string lampFragmentShaderSource =
		"#version 330 core\n"
		"out vec4 fragColor;"
		"void main()\n"
		"{\n"
		"fragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);"
		"}\n";


	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource); // Creating Shader Program
	GLuint lampShaderProgram = CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource); // Creating Light Shader Program

	/* SHADERS END */

	
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Resize window and graphics simultaneously
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//change window background color
		//glClearColor(0.73f, 0.65f, 0.56f, 1.0f);


		// Use Shader Program exe and select VAO before drawing 
		glUseProgram(shaderProgram); // Call Shader per-frame when updating attributes

			// Declare transformations (can be initialized outside loop)		
			glm::mat4 projectionMatrix;

			// Define LookAt Matrix
			viewMatrix = glm::lookAt(cameraPosition, target, worldUp);

			// Define projection matrix
			projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

			// Setup views and projections
			if (ortho) {
				GLfloat oWidth = (GLfloat)width * 0.01f; // 10% of width
				GLfloat oHeight = (GLfloat)height * 0.01f; // 10% of height
				viewMatrix = glm::lookAt(cameraPosition, target, -worldUp);
				projectionMatrix = glm::ortho(-oWidth, oWidth, oHeight, -oHeight, 0.1f, 100.0f);
			}
			else {
				viewMatrix = glm::lookAt(cameraPosition, target, worldUp);
				projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
			}

			// Get matrix's uniform location and set matrix
			GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
			GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
			GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

			//Get light and object color, light and view position locations
			GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
			GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
			GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
			GLint lightTwoColorLoc = glGetUniformLocation(shaderProgram, "lightTwoColor");
			GLint lightTwoPosLoc = glGetUniformLocation(shaderProgram, "lightTwoPos");
			GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

			//Assign Object and Light Colors  
			glUniform3f(objectColorLoc, 0.46f, 0.36f, 0.25f); //Get RGB values from online program (RBG# divided by 255) 242,203,2
			glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); //white light
			glUniform3f(lightTwoColorLoc, 1.0f, 1.0f, 0.9f); //yellow light

			//Set light position
			glUniform3f(lightPosLoc, lightPosition.x, lightPosition.y, lightPosition.z); 
			glUniform3f(lightTwoPosLoc, lightTwoPosition.x, lightTwoPosition.y, lightTwoPosition.z); 

			//Specifiy view position
			glUniform3f(viewPosLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));


		/* Question Cube */

		glBindTexture(GL_TEXTURE_2D, marioTexture); 
		glBindVertexArray(cubeVAO); // Active VAO

			// Transform planes to form cube
			for (GLuint i = 0; i < 6; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.0f, 0.0f)); // Position  
				modelMatrix = glm::translate(modelMatrix, planePositions[i]);
				modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}

		glBindVertexArray(0); //Incase different VAO wii be used after

		/* End Cube */

		/* Table */
		glBindTexture(GL_TEXTURE_2D, tableTexture);

		// Select and transform table
		glBindVertexArray(tableVAO); // Active VAO
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f, -0.51f, 0.0f)); // Position 
			modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(10.0f, 8.0f, 10.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		glBindVertexArray(0); //Incase different VAO will be used after

		/* End Table */

		/* Pencil Tip */

		glBindTexture(GL_TEXTURE_2D, pencilTipTexture);
		glBindVertexArray(triangleVAO); // Active VAO

			// Use loop to build Model matrix for Pencil Tip
			for (GLuint i = 0; i < 6; i++)
			{
				glm::mat4 modelMatrix;
				// Apply Transform to model // Build model matrix for Pencil Tip
				modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.0f, -.42f, 0.25f)); // Position 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(-110.f), glm::vec3(0.0f, 1.0f, 0.0f)); 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(triRotations[i]), glm::vec3(0.0f, 0.0f, 1.0f)); 
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.07f, 0.07f, 0.07f)); 
				
				//Copy perspective and MV matrices to uniforms
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();

			}
		// Unbind Shader exe and VOA after drawing per frame
		glBindVertexArray(0); //Incase different VAO wii be used after

		/* End Pencil Tip */

		/* Pencil Base */

		glBindTexture(GL_TEXTURE_2D, pencilBodyTexture);
		glBindVertexArray(cylinderVAO); // Activate VAO 

			// Use loop to build Model matrix for  Pencil Base
			for (int i = 0; i < 6; i++) {
				glm::mat4 modelMatrix;
				// Apply Transform to model // Build model matrix for  Pencil Base
				modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.0f, -.42f, 0.25f)); // Position  
				modelMatrix = glm::rotate(modelMatrix, glm::radians(-110.f), glm::vec3(0.0f, 1.0f, 0.0f)); 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(triRotations[i]), glm::vec3(0.0f, 0.0f, 1.0f)); 
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.07f, 0.07f, 0.07f)); 

				//Copy perspective and MV matrices to uniforms
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

				glDrawArrays(GL_TRIANGLES, 0, 12); // Render primitive or execute shader per draw
			}
		
		glBindVertexArray(0); //Incase different VAO wii be used after

		/* End Pencil Base */

		/* Ball */
		glBindTexture(GL_TEXTURE_2D, silverTexture);
		glBindVertexArray(sphereVAO); // Activate VAO 

			// Use loop to build Model matrix for  Pencil Base
			for (int i = 0; i < 6; i++) {
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, glm::vec3(1.30f, -0.23f, 0.5f)); // Position 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(-80.f), glm::vec3(1.0f, 0.0f, 0.0f)); 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(triRotations[i]), glm::vec3(0.0f, 0.0f, 1.0f)); 
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 0.3f)); 

				//Copy perspective and MV matrices to uniforms
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

				glDrawArrays(GL_TRIANGLES, 0, 30); // Render primitive or execute shader per draw
			}

		glBindVertexArray(0); 

		/* End Ball */

		/* Ring */

		glBindTexture(GL_TEXTURE_2D, pinkStripeTexture);
		glBindVertexArray(torusVAO); // Activate VAO 

			for (int i = 0; i < 12; i++) {
				glm::mat4 modelMatrix;

				modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f, -0.44f, 1.1f)); // Position 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.f), glm::vec3(1.0f, 0.0f, 0.0f)); 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(torusRotations[i]), glm::vec3(0.0f, 0.0f, 1.0f)); 
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.15f, 0.15f, 0.15f));

				//Copy perspective and MV matrices to uniforms
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

				glDrawArrays(GL_TRIANGLES, 0, 36); // Render primitive or execute shader per draw
			}

		glBindVertexArray(0); 

		/* End Ball */

		glUseProgram(0); // Incase different shader will be used after


		glUseProgram(lampShaderProgram);

			// Get matrix's uniform location and set matrix
			GLint lampModelLoc = glGetUniformLocation(lampShaderProgram, "model");
			GLint lampViewLoc = glGetUniformLocation(lampShaderProgram, "view");
			GLint lampProjLoc = glGetUniformLocation(lampShaderProgram, "projection");

			glUniformMatrix4fv(lampViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
			glUniformMatrix4fv(lampProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

			glBindVertexArray(lampVAO); // Active VAO

			for (GLuint i = 0; i < 6; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] / glm::vec3(8., 8., 8.) + lightPosition);
				modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.125f, 0.125f, 0.125f));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}

			// Unbind Shader exe and VOA after drawing per frame
			glBindVertexArray(0); //Incase different VAO wii be used after

			glBindVertexArray(lampVAO); // Active VAO

			for (GLuint i = 0; i < 6; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] / glm::vec3(8., 8., 8.) + lightTwoPosition);
				modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.125f, 0.125f, 0.125f));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}

			// Unbind Shader exe and VOA after drawing per frame
			glBindVertexArray(0); //Incase different VAO wii be used after

		glUseProgram(0); // Incase different shader will be used after

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		// Poll Camera Transformations
		TransformCamera();

	}

	//Clear GPU resources
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &cubeEBO);
	glDeleteVertexArrays(1, &tableVAO);
	glDeleteBuffers(1, &tableVBO);
	glDeleteBuffers(1, &tableEBO);
	glDeleteVertexArrays(1, &triangleVAO);
	glDeleteBuffers(1, &triangleVBO);
	glDeleteBuffers(1, &triangleEBO);
	glDeleteVertexArrays(1, &cylinderVAO);
	glDeleteBuffers(1, &cylinderVBO);
	glDeleteBuffers(1, &cylinderEBO);
	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereEBO);
	glDeleteVertexArrays(1, &torusVAO);
	glDeleteBuffers(1, &torusVBO);
	glDeleteBuffers(1, &torusEBO);
	glDeleteVertexArrays(1, &lampVAO);
	glDeleteBuffers(1, &lampVBO);
	glDeleteBuffers(1, &lampEBO);

	glfwTerminate();
	return 0;
}

// Define input functions
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// Display ASCII Key code
	//std::cout <<"ASCII: "<< key << std::endl;	

	// Close window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Assign true to Element ASCII if key pressed
	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE) // Assign false to Element ASCII if key released
		keys[key] = false;

}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{

	// Clamp FOV
	if (fov >= 1.0f && fov <= 55.0f)
		fov -= yoffset * 0.01;

	// Default FOV
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 55.0f)
		fov = 55.0f;

}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

	if (firstMouseMove)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}
	// Calculate mouse offset (Easing effect)
	xChange = xpos - lastX;
	yChange = lastY - ypos; // Inverted cam

	// Get current mouse (always starts at 0)
	lastX = xpos;
	lastY = ypos;


	if (isOrbiting)
	{
		// Update raw yaw and pitch with mouse movement
		rawYaw += xChange;
		rawPitch += yChange;

		// Conver yaw and pitch to degrees, and clamp pitch
		degYaw = glm::radians(rawYaw);
		degPitch = glm::clamp(glm::radians(rawPitch), -glm::pi<float>() / 2.f + .1f, glm::pi<float>() / 2.f - .1f);

		// Azimuth Altitude formula
		cameraPosition.x = target.x + radius * cosf(degPitch) * sinf(degYaw);
		cameraPosition.y = target.y + radius * sinf(degPitch);
		cameraPosition.z = target.z + radius * cosf(degPitch) * cosf(degYaw);
	}
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mode)
{
	// Assign boolean state to element Button code
	if (action == GLFW_PRESS)
		mouseButtons[button] = true;
	else if (action == GLFW_RELEASE)
		mouseButtons[button] = false;
}



// Define TransformCamera function
void TransformCamera()
{

	// Orbit camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButtons[GLFW_MOUSE_BUTTON_LEFT])
		isOrbiting = true;
	else
		isOrbiting = false;

	// Focus camera
	if (keys[GLFW_KEY_F])
		initiateCamera();

	if (keys[GLFW_KEY_W])
		cameraPosition += cameraSpeed * cameraFront;
	if (keys[GLFW_KEY_S])
		cameraPosition -= cameraSpeed * cameraFront;
	if (keys[GLFW_KEY_A])
		cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (keys[GLFW_KEY_D])
		cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (keys[GLFW_KEY_Q])
		cameraPosition += cameraSpeed * cameraUp;
	if (keys[GLFW_KEY_E])
		cameraPosition -= cameraSpeed * cameraUp;

	// Toggle orthographic projection
	if (keys[GLFW_KEY_O])
		ortho = true;
	if (keys[GLFW_KEY_P])
		ortho = false;
}

// Define 
void initiateCamera()
{	// Define Camera Attributes
	cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f); // Move 3 units back in z towards screen
	target = glm::vec3(0.0f, 0.0f, 0.0f); // What the camera points to
	cameraDirection = glm::normalize(cameraPosition - cameraDirection); // direction z
	worldUp = glm::vec3(0.0, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));// right vector x
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight)); // up vector y
	cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // 1 unit away from lense
}
