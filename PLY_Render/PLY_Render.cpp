#include <iostream>
#include <chrono>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi
#include<glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
//#include <glm/mat4>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <string>
#include <happly.h>
#include<vector>
#include<png.h>
#include <errno.h>


#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
using namespace std;

int width = 1164;
int height = 765;

std::vector<std::array<double, 3>> vPos;
std::vector<std::array<unsigned char, 3>> vCol;

static GLubyte* pixels = NULL;

//Image Index
int imageIndex = 2;
int passIndex = 0;
bool loaded = false; //allow render to load for 1 iteration
//Text Parser
const int set_size = 100;
float QW[set_size];
float QX[set_size];
float QY[set_size];
float QZ[set_size];
float TX[set_size];
float TY[set_size];
float TZ[set_size];
string names[set_size];

//String helper methods 
template <typename Out>
void split(const std::string& s, char delim, Out result) {
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim)) {
		*result++ = item;
	}
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}


static png_byte* png_bytes = NULL;
static png_byte** png_rows = NULL;
FILE* filepoint;
errno_t err;
static void screenshot_png(const char* filename, unsigned int width, unsigned int height,
	GLubyte** pixels, png_byte** png_bytes, png_byte*** png_rows) {
	size_t i, nvals;
	const size_t format_nchannels = 4;
	err = fopen_s(&filepoint,filename, "wb");
	nvals = format_nchannels * width * height;
	*pixels = (GLubyte*)realloc(*pixels, nvals * sizeof(GLubyte));
	*png_bytes = (png_byte*)realloc(*png_bytes, nvals * sizeof(png_byte));
	*png_rows = (png_byte**)realloc(*png_rows, height * sizeof(png_byte*));
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, *pixels);
	for (i = 0; i < nvals; i++)
		(*png_bytes)[i] = (*pixels)[i];
	for (i = 0; i < height; i++)
		(*png_rows)[height - i - 1] = &(*png_bytes)[i * width * format_nchannels];
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) abort();
	png_infop info = png_create_info_struct(png);
	if (!info) abort();
	if (setjmp(png_jmpbuf(png))) abort();
	png_init_io(png, filepoint);
	png_set_IHDR(
		png,
		info,
		width,
		height,
		8,
		PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);
	png_write_info(png, info);
	png_write_image(png, *png_rows);
	png_write_end(png, NULL);
	png_destroy_write_struct(&png, &info);
	fclose(filepoint);
}


void loadImageTXT() {
	ifstream inFile;
	inFile.open("D:\\Downloads\\gerrard-hall\\gerrard-hall\\sparse\\images.txt");

	if (!inFile) {
		cerr << "Unable to open file datafile.txt";
		exit(1);   // call system to stop
	}


	int count = 0;
	for (std::string line; getline(inFile, line); )
	{

		if (line.find("JPG") != std::string::npos) {

			std::vector<std::string> x = split(line, ' ');
			QW[count] = atof(x[1].c_str());
			QX[count] = atof(x[2].c_str());
			QY[count] = atof(x[3].c_str());
			QZ[count] = atof(x[4].c_str());
			TX[count] = atof(x[5].c_str());
			TY[count] = atof(x[6].c_str());
			TZ[count] = atof(x[7].c_str());
			names[count] = x[9];
			count++;
		}
	}
}


void changeSize(int w, int h) {

	if (h == 0)
		h = 1;

	float ratio = w * 1.0 / h;

	glMatrixMode(GL_PROJECTION);

	// Reset Matrix
	glLoadIdentity();

	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);

	//gluOrtho(0.0f, w, h, 0.0f, 0.0f, 1.0f);
	//gluOrtho2D(0.0, 900.0, 0.0, 640.0);
	// Set the correct perspective.
	gluPerspective(52.01f, ratio, 0.1f, 100.0f);

	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}



void drawPointCloud() {
	glPointSize(2.0f);
	glBegin(GL_POINTS);
	for (std::vector<std::array<double, 3>>::size_type i = 0; i != vPos.size(); i++) {
		glColor3f((float)vCol[i][0] / 256, (float)vCol[i][1] / 256, (float)vCol[i][2] / 256);
		glVertex3f(vPos[i][0], vPos[i][1], vPos[i][2]);
	}
	glEnd();
}

void drawPointCloudDepth(glm::vec3 camPos) {
	glPointSize(2.0f);
	
	glBegin(GL_POINTS);
	for (std::vector<std::array<double, 3>>::size_type i = 0; i != vPos.size(); i++) {
		float distance = (float)glm::distance(camPos, glm::vec3(vPos[i][0], vPos[i][1], vPos[i][2]));

		glColor3f(1-(distance / 6), 1-(distance / 6), 1-(distance / 6));
		glVertex3f(vPos[i][0], vPos[i][1], vPos[i][2]);
	}
	glEnd();
}



void renderScene(void) {
	//set background color
	if (passIndex == 0) {
		glClearColor(1, 1, 1, 1);
	}
	else {
		glClearColor(0, 0, 0, 1);
	}

	// Clear Color and Depth Buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Reset transformations
	glLoadIdentity();
	// Set the camera


	glm::quat orientation = glm::quat(QW[imageIndex], QX[imageIndex], QY[imageIndex], QZ[imageIndex]);
	glm::mat3 RotationMatrix = glm::toMat3(orientation);
	glm::mat3 Inverse = glm::inverse(RotationMatrix);
	glm::vec3 position = glm::vec3(TX[imageIndex], TY[imageIndex], TZ[imageIndex]);

	glm::vec3 newPos = -Inverse * position;

	glm::vec3 defaultUpVector = glm::vec3(0, -1, 0);
	glm::vec3 defaultForwardVector = glm::vec3(0, 0, 1);

	glm::vec3 pUpVector = Inverse * defaultUpVector ;
	glm::vec3 pLookAt = newPos + (Inverse* defaultForwardVector); 

	gluLookAt(newPos.x, newPos.y, newPos.z,
		pLookAt.x, pLookAt.y, pLookAt.z,
		pUpVector.x, pUpVector.y, pUpVector.z);
		
	std::string name;
	if (passIndex == 0) {
		drawPointCloud();
		name = "D:\\nerual\\output\\color\\" + names[imageIndex].substr(4, names[imageIndex].size() - 8) + "_color.png";
	}
	else {
		drawPointCloudDepth(newPos);
		name = "D:\\nerual\\output\\depth\\" + names[imageIndex].substr(4, names[imageIndex].size() - 8) + "_depth.png";
	}
	
	glutSwapBuffers();
	

	
	
	screenshot_png(name.c_str(), width, height, &pixels, &png_bytes, &png_rows);

	if (loaded) {
		if (imageIndex < set_size - 1) {
			imageIndex++;
		}
		else if (passIndex == 0) {
			passIndex = 1;
			imageIndex = 0;
		}
		loaded = false;
	}
	else {
		loaded = true;
	}
	
	
}




int main(int argc, char** argv) {

	loadImageTXT();
	cout << names[imageIndex];
	//load point cloud file
	happly::PLYData plyIn("D:/nerual/fused.ply");
	vPos = plyIn.getVertexPositions();
	vCol = plyIn.getVertexColors();

	// init GLUT and create window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("Fluid");


	//glEnable(GL_MULTISAMPLE);

	//camera.SetPosition(glm::vec3(0, 0, -1));
	//camera.SetLookAt(glm::vec3(0, 0, 0));
	//camera.SetClipping(.1, 1000);
	//camera.SetFOV(45);

	// register callbacks
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);

	glutIgnoreKeyRepeat(1);
	//glutKeyboardFunc(processNormalKeys);
	//glutSpecialFunc(pressKey);
	//glutSpecialUpFunc(releaseKey);

	//// here are the two new functions
	//glutMouseFunc(mouseButton);
	//glutMotionFunc(mouseMove);

	// OpenGL init
	glEnable(GL_DEPTH_TEST);

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}

