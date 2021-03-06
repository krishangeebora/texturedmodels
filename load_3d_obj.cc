/*
This program demonstrates the basic steps for loading a 3D model using Assimp and display it. 
There is no transformation, lighting, or texture mapping. 

This program needs the following libraries to run:
	Freeglut
	Glew
	Assimp

There is no need to load external shaders. They are embedded in the program. 

Modify the path of the 3D object file for your computer. 

Ying Zhu
Department of Computer Science
Georgia State University

2014

*/

#include <fstream>
#include <iostream>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "assimp/Importer.hpp"
#include "assimp/PostProcess.h"
#include "assimp/Scene.h"

// This header file contains utility functions that print out the content of 
// aiScene data structure. 
#include "check_error.hpp"
#include "assimp_utilities.hpp"

using namespace std;

#define BUFFER_OFFSET( offset ) ((GLvoid*) offset)

//--------------------------------------------------------
// Shader related variables

// Vertex shader source code
const char* vShader = {
		"#version 330\n"
		"in vec3 vPos;"
		""
		"void main() {"
		" gl_Position = vec4(vPos, 1);"
		"}"
	};

// Fragment shader source code
const char* fShader = {
		"#version 330\n"
		"out vec4 fColor;"
		""
		"void main() {"
		" fColor = vec4(0.0, 0.0, 0.0, 1.0);"
		"}"
	};

// Index of the shader program
GLuint program;

// Index of the in variable vPos in the vertex shader
GLint vPos;

//-------------------------------------------------
// 3D object related variable

// Make sure you modify the path of the 3D object file for your computer. 
const char * objectFileName = "Models\\dog3.dae";

// This is the Assimp importer object that loads the 3D file. 
Assimp::Importer importer;

// Global Assimp scene object
const aiScene* scene = NULL;

// This array stores the VAO indices for each corresponding mesh in the aiScene object. 
// For example, vaoArray[0] stores the VAO index for the mesh scene->mMeshes[0], and so on. 
unsigned int *vaoArray = NULL;

// This is the 1D array that stores the face indices of a mesh. 
unsigned int *faceArray = NULL;

//---------------------------------------
// Functions

// Load a 3D file
bool load3DFile( const char *filename) {

	ifstream fileIn(filename);

	// Check if the file exists. 
	if (fileIn.good()) {
		fileIn.close();  // The file exists. 
	} else {
		fileIn.close();
		cout << "Unable to open the 3D file." << endl;
		return false;
	}

	cout << "Loading 3D file " << filename << endl;

	// Load the 3D file using Assimp. The content of the 3D file is stored in an aiScene object. 
	scene = importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Quality);

	// Check if the file is loaded successfully. 
	if(!scene)
	{
		// Fail to load the file
		cout << importer.GetErrorString() << endl;
		return false;
	} else {
		cout << "3D file " << filename << " loaded." << endl;
	}

	// This is optional. Print the content of the aiScene object, if needed. 
	// This function is defined in the print_aiscene.hpp file. 
	printAiSceneInfo(scene);

	return true;
}

//Prepare the shaders and 3D data
void init()
{
	// ---------------------------
	// Load and build the shaders. 

	GLuint vShaderID, fShaderID;

	// Create empty shader objects
	vShaderID = glCreateShader(GL_VERTEX_SHADER);
	if (vShaderID == 0) {
		cout << "There is an error creating the vertex shader." << endl;
	}

	fShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	if (fShaderID == 0) {
		cout << "There is an error creating the fragment shader." << endl;
	}

	// Attach shader source code the shader objects
	glShaderSource(vShaderID, 1, &vShader, NULL);
	glShaderSource(fShaderID, 1, &fShader, NULL);

	// Compile the vertex shader object
	glCompileShader(vShaderID);
	printShaderInfoLog(vShaderID); // Print error messages, if any. 

	// Compile the fragment shader object
	glCompileShader(fShaderID);
	printShaderInfoLog(fShaderID); // Print error messages, if any. 

	// Create an empty shader program object
	program = glCreateProgram();
	if (program == 0) {
		cout << "There is an error creating the shader program." << endl;
	}

	// Attach vertex and fragment shaders to the shader program
	glAttachShader(program, vShaderID);
	glAttachShader(program, fShaderID);

	// Link the shader program
	glLinkProgram(program);
	// Check if the shader program can run in the current OpenGL state, just for testing purposes. 
	glValidateProgram(program);
	printShaderProgramInfoLog(program); // Print error messages, if any. 

	// glGetAttribLocation() should not be called before glLinkProgram()
	// because the shader variables haven't been bound before the program is linked. 
	vPos = glGetAttribLocation( program, "vPos" );
	if (vPos == -1) {
		cout << "There is an error when calling glGetAttribLocation()." << endl; 
	}

	// -------------------
	// Load the 3D file
	
	// This variable temporarily stores the VBO index. 
	GLuint buffer;

	// Load the 3D file using Assimp.
	bool fileLoaded = load3DFile(objectFileName);
	if (!fileLoaded) {
		return;
	}

	// Create an array to store the VAO indices for each mesh. 
	vaoArray = (unsigned int*) malloc(sizeof(unsigned int) * scene->mNumMeshes);

	// Go through each mesh stored in the aiScene object, bind it with a VAO, 
	// and save the VAO index in the vaoArray. 
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* currentMesh = scene->mMeshes[i];

		// Create an empty Vertex Array Object (VAO). VAO is only available from OpenGL 3.0 or higher. 
		// Note that the vaoArray[] index is in sync with the mMeshes[] array index. 
		// That is, for mesh #0, the corresponding VAO index is stored in vaoArray[0], and so on. 
		glGenVertexArrays(1, &vaoArray[i]);
		glBindVertexArray(vaoArray[i]);

		if (currentMesh->HasPositions()) {
			// Create an empty Vertex Buffer Object (VBO)
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);

			// Bind (transfer) the vertex position array (stored in aiMesh's member variable mVertices) 
			// to the VBO.
			// Note that the vertex positions are stored in a continuous 1D array (i.e. mVertices) in the aiScene object. 
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * currentMesh->mNumVertices, 
				currentMesh->mVertices, GL_STATIC_DRAW);

			// Associate this VBO with an the vPos variable in the vertex shader. 
			// The vertex data and the vertex shader must be connected. 
			glEnableVertexAttribArray( vPos );
			glVertexAttribPointer( vPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
		} 

		if (currentMesh->HasFaces()) {
			// Create an array to store the face indices (elements) of this mesh. 
			// This is necessary becaue face indices are NOT stored in a continuous 1D array inside aiScene. 
			// Instead, there is an array of aiFace objects. Each aiFace object stores a number of (usually 3) face indices.
			// We need to copy the face indices into a continuous 1D array. 
			faceArray = (unsigned int*)malloc(sizeof(unsigned int) * currentMesh->mNumFaces * currentMesh->mFaces[0].mNumIndices);

			// copy the face indices from aiScene into a continuous 1D array faceArray.  
			int faceArrayIndex = 0;
			for (unsigned int j = 0; j < currentMesh->mNumFaces; j++) {
					for (unsigned int k = 0; k < currentMesh->mFaces[j].mNumIndices; k++) {
						faceArray[faceArrayIndex] = currentMesh->mFaces[j].mIndices[k]; 
						faceArrayIndex++;
					}
			}

			// Create an empty VBO
			glGenBuffers(1, &buffer);

			// This VBO is an GL_ELEMENT_ARRAY_BUFFER, not a GL_ARRAY_BUFFER. 
			// GL_ELEMENT_ARRAY_BUFFER stores the face indices (elements), while 
			// GL_ARRAY_BUFFER stores vertex positions. 
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
				sizeof(unsigned int) * currentMesh->mNumFaces * currentMesh->mFaces[0].mNumIndices, 
				faceArray, GL_STATIC_DRAW);
		}

		//Close the VAOs and VBOs for later use. 
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	}

	// Turn on visibility test. 
	glEnable(GL_DEPTH_TEST);

	// Draw the object in wire frame mode. 
	// You can comment out this line to draw the object in 
	// shaded mode. But without lighting, the object will look very dark. 
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// This is the background color. 
	glClearColor( 1.0, 1.0, 1.0, 1.0 );

	// Uncomment this line for debugging purposes. 
	checkOpenGLError("init()");
}

// Traverse the node tree in the aiScene object and draw the meshes associated with each node. 
// This function is called recursively to perform a depth-first tree traversal. 
void nodeTreeTraversal(const aiNode* node) {
	if (!node) {
		cout << "nodeTreeTraversal(): Null node" << endl;
		return; 
	}

	// Draw all the meshes associated with the current node.
	// Certain node may have no mesh associated with it. 
	// Certain node may have multiple meshes associated with it. 
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		// This is the index of the mesh associated with this node.
		int meshIndex = node->mMeshes[i];   

		// This mesh should have already been associated with a VAO in a previous function. 
		// Note that mMeshes[] array and the vaoArray[] array are in sync. 
		// That is, for mesh #0, the corresponding VAO index is stored in vaoArray[0], and so on. 
		// Bind the corresponding VAO for this mesh. 
		glBindVertexArray(vaoArray[meshIndex]); 
 
		const aiMesh* currentMesh = scene->mMeshes[meshIndex];
		// How many faces are in this mesh?
		unsigned int numFaces = currentMesh->mNumFaces; 
		// How many indices are for each face?
		unsigned int numIndicesPerFace = currentMesh->mFaces[0].mNumIndices;

		// The second parameter is crucial. This is the number of face indices, not the number of faces.
		// (numFaces * numIndicesPerFace) is the total number of elements(face indices) of this mesh.
		// Now draw all the faces. We know these faces are triangle because in 
		// importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Quality);
		// "aiProcessPreset_TargetRealtime_Quality" indicates that the 3D object will be triangulated. 
		glDrawElements(GL_TRIANGLES, (numFaces * numIndicesPerFace), GL_UNSIGNED_INT, 0);

		// We are done with the current VAO. Move on to the next VAO, if any. 
		glBindVertexArray(0); 
	}

	// Uncomment this line for debugging purposes. 
	// checkOpenGLError();

	// Recursively visit and draw all the child nodes. This is a depth-first traversal. 
	for (unsigned int j = 0; j < node->mNumChildren; j++) {
		nodeTreeTraversal(node->mChildren[j]); 
	}
}

void display() {
	// Clear the background color and the depth buffer. 
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Activate the shader program. 
	glUseProgram( program );

	// Start the node tree traversal and process each node. 
	if (scene) {
		nodeTreeTraversal(scene->mRootNode);
	}

	// Swap front and back buffers. The rendered image is now displayed. 
	glutSwapBuffers();
}


// This function is called when the size of the window is changed. 
void reshape( int width, int height )
{
	// Specify the size and location of the rendered picture. 
	glViewport( 0, 0, width, height );
}

// This function is called when a key is pressed. 
void keyboard( unsigned char key, int x, int y )
{
	switch( key ) {
		case 033: // Escape Key
			exit( EXIT_SUCCESS );
			break;
	}

	// Generate a display event, which forces Freeglut to call display(). 
	glutPostRedisplay();
}

int main( int argc, char* argv[] )
{
	glutInit( &argc, argv );

	// Initialize double buffer and depth buffer. 
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	// If you don't specify an OpenGL context version,
	// a default OpenGL context and profile will be created. 
	// Must explicity specify OpenGL context 4.3 for the debug callback function to work. 
	// glutInitContextVersion( 4, 3 );
	// glutInitContextFlags(GLUT_DEBUG);

	glutCreateWindow( argv[0] );

	// Initialize Glew. This must be called before any OpenGL function call. 
	glewInit();

	// These cannot be called before glewInit().
	cout << "OpenGL version " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Shading Language version " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl << endl;

	// Usually you want to enable GL_DEBUG_OUTPUT_SYNCHRONOUS so that error messages are immediately reported. 
	// glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	// Register a debug message callback function. 
	// glDebugMessageCallback((GLDEBUGPROC)openGLDebugCallback, nullptr);

	// Uncomment this line for debugging purposes. 
	checkOpenGLError("main()");

	init();

	glutDisplayFunc( display );

	glutReshapeFunc( reshape );

	glutKeyboardFunc( keyboard );

	glutMainLoop();

	//Release the dynamically allocated memory blocks. 
	free(vaoArray);
	free(faceArray);
}