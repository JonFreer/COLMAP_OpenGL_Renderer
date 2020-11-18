#include <iostream>
#include <cmath>
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
#include <map>


#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
using namespace std;

//Desnse Point Cloud Data
std::vector<std::array<double, 3>> vPos;
std::vector<std::array<unsigned char, 3>> vCol;

static GLubyte* pixels = NULL;

//Image Index
int imageIndex = 0;
int passIndex = 0; //0 for color, 1 for depth
int frameCount = 0; //work around to allow frame updates before saving

//variables
const int set_size = 1363;
const string imagePath = "D:\\Downloads\\dense\\sparse\\images.txt";
const string cameraPath = "D:\\Downloads\\dense\\sparse\\cameras.txt";
const string plyPath = "D:/Downloads/fused_backup.ply";
const string colorPath = "D:\\nerual\\output2\\color\\";
const string depthPath = "D:\\nerual\\output2\\depth\\";
const float depthThresh = 15;

struct Image {
	string name;
	int cameraID;
	float QW;
	float QX;
	float QY;
	float QZ;
	float TX;
	float TY;
	float TZ;
};

struct Camera {
	int height;
	int width;
	float focalY;
};

std::map<int, Camera> cameras;
Image images[set_size];

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

static void screenshot_png_grey(const char* filename, unsigned int width, unsigned int height,
	GLubyte** pixels, png_byte** png_bytes, png_byte*** png_rows) {
	size_t i, nvals;
	const size_t format_nchannels = 1;

	err = fopen_s(&filepoint, filename, "wb");
	nvals = format_nchannels * width * height;
	*pixels = (GLubyte*)realloc(*pixels, nvals *4* sizeof(GLubyte));
	*png_bytes = (png_byte*)realloc(*png_bytes, nvals * sizeof(png_byte));
	*png_rows = (png_byte**)realloc(*png_rows, height * sizeof(png_byte*));
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, *pixels);

	for (i = 0; i < nvals; i++)
		(*png_bytes)[i] = (*pixels)[i*4]; //just take the r value
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
		PNG_COLOR_TYPE_GRAY,
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

void loadCameraTXT() {
	ifstream inFile;
	inFile.open(cameraPath);

	if (!inFile) {
		cerr << "Unable to open file datafile.txt";
		exit(1);   // call system to stop
	}

	int count = 0;
	for (std::string line; getline(inFile, line); )
	{

		if (line.find("OPENCV") != std::string::npos || line.find("PINHOLE") != std::string::npos) {

			std::vector<std::string> x = split(line, ' ');
			int id = atoi(x[0].c_str());
			cameras[id] = Camera();
			cameras[id].width = atoi(x[2].c_str());
			cameras[id].height = atoi(x[3].c_str());
			cameras[id].focalY = atof(x[5].c_str());
			count++;
		}
	}

}


void loadImageTXT() {
	ifstream inFile;
	inFile.open(imagePath);

	if (!inFile) {
		cerr << "Unable to open file datafile.txt";
		exit(1);   // call system to stop
	}


	int count = 0;
	for (std::string line; getline(inFile, line); )
	{

		if (line.find("jpg") != std::string::npos || line.find("JPG") != std::string::npos) {

			std::vector<std::string> x = split(line, ' ');
			images[count] = Image();
			images[count].QW = atof(x[1].c_str());
			images[count].QX = atof(x[2].c_str());
			images[count].QY = atof(x[3].c_str());
			images[count].QZ = atof(x[4].c_str());
			images[count].TX = atof(x[5].c_str());
			images[count].TY = atof(x[6].c_str());
			images[count].TZ = atof(x[7].c_str());
			images[count].cameraID = atoi(x[8].c_str());
			images[count].name = x[9];
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
	glutReshapeWindow(w, h);

	// Set the correct perspective.
	float temp = (h / 2) / cameras[images[imageIndex].cameraID].focalY;
	float fov = 2*(atan(temp) * 180 / 3.1415);
	
	gluPerspective(fov, ratio, 0.1f, 100.0f);

	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}



void drawPointCloud() {
	glPointSize(cameras[images[imageIndex].cameraID].width/500);
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

		glColor3f((distance / depthThresh), (distance / depthThresh), (distance / depthThresh));
		glVertex3f(vPos[i][0], vPos[i][1], vPos[i][2]);
	}
	glEnd();
}



void renderScene(void) {
	
	int height = cameras[images[imageIndex].cameraID].height;
	int width = cameras[images[imageIndex].cameraID].width;
	if (frameCount == 0) {
		changeSize(width, height);
	}
	
	//set background color
	if (passIndex == 0) {
		glClearColor(1, 1, 1, 1);
	}
	else {
		glClearColor(1, 1, 1, 1);
	}

	// Clear Color and Depth Buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Reset transformations
	glLoadIdentity();
	// Set the camera


	glm::quat orientation = glm::quat(images[imageIndex].QW, images[imageIndex].QX, images[imageIndex].QY, images[imageIndex].QZ);
	glm::mat3 RotationMatrix = glm::toMat3(orientation);
	glm::mat3 Inverse = glm::inverse(RotationMatrix);
	glm::vec3 position = glm::vec3(images[imageIndex].TX, images[imageIndex].TY, images[imageIndex].TZ);

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
		name = colorPath + images[imageIndex].name.substr(0, images[imageIndex].name.size() - 4) + ".png";
	}
	else {
		drawPointCloudDepth(newPos);
		name = depthPath + images[imageIndex].name.substr(0, images[imageIndex].name.size() - 4) + ".png";
	}
	
	glutSwapBuffers();
	if (frameCount == 3||frameCount == 4) {


		if (passIndex == 0) {
			screenshot_png(name.c_str(), width, height, &pixels, &png_bytes, &png_rows);
		}
		else {
			screenshot_png_grey(name.c_str(), width, height, &pixels, &png_bytes, &png_rows);
		}
	}
	
	
	if (frameCount == 5) {
		if (imageIndex < set_size - 1) {
			imageIndex++;
		}
		else if (passIndex == 0) {
			passIndex = 1;
			imageIndex = 0;
		}
		frameCount = 0;
	}
	else {
		frameCount++;
	}

	
}


int main(int argc, char** argv) {

	loadImageTXT();
	loadCameraTXT();

	happly::PLYData plyIn(plyPath);
	vPos = plyIn.getVertexPositions();
	vCol = plyIn.getVertexColors();

	// init GLUT and create window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(100, 100);
	glutCreateWindow("Fluid");

	// register callbacks
	glutDisplayFunc(renderScene);
	//glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);

	glutIgnoreKeyRepeat(1);

	// OpenGL init
	glEnable(GL_DEPTH_TEST);

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}

