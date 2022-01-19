//
//  RayCaster - Set of simple classes to create a camera/view setup for our Ray Tracer HW Project
//
//  I've included these classes as a mini-framework for our introductory ray tracer.
//  You are free to modify/change.   
//
//  These classes provide a simple render camera which can can return a ray starting from
//  it's position to a (u, v) coordinate on the view plane.
//
//  The view plane is where we can locate our photorealistic image we are rendering.
//  The field-of-view of the camera by moving it closer/further 
//  from the view plane.  The viewplane can be also resized.  When ray tracing an image, the aspect
//  ratio of the view plane should the be same as your image. So for example, the current view plane
//  default size is ( 6.0 width by 4.0 height ).   A 1200x800 pixel image would have the same
//  aspect ratio.
//
//  This is not a complete ray tracer - just a set of skelton classes to start.  The current
//  base scene object only stores a value for the diffuse/specular color of the object (defaut is gray).
//  at some point, we will want to replace this with a Material class that contains these (and other 
//  parameters)
//  
//  (c) Kevin M. Smith  - 24 September 2018
//
#pragma once

/*

	Elston Ma
	CS 116A

*/

#include "ofMain.h"
#include "ofxGui.h"
#include <glm/gtx/intersect.hpp>

//  General Purpose Ray class 
//
class Ray {
public:
	Ray(glm::vec3 p, glm::vec3 d) { this->p = p; this->d = d; }
	void draw(float t) { ofDrawLine(p, p + t * d); }

	glm::vec3 evalPoint(float t) {
		return (p + t * d);
	}

	glm::vec3 p, d;
};

//  Base class for any renderable object in the scene
//
class SceneObject {
public: 
	virtual void draw() = 0;    // pure virtual funcs - must be overloaded
	virtual bool intersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) { cout << "SceneObject::intersect" << endl; return false; }
	virtual void applyTexture(glm::vec3 appPt) = 0;
	virtual void loadTexture(ofImage aTexture) = 0;

	// give scene object a texture if applicable
	ofImage texture;
	
	// any data common to all scene objects goes here
	glm::vec3 position = glm::vec3(0, 0, 0);

	// material properties (we will ultimately replace this with a Material class - TBD)
	//
	ofColor diffuseColor = ofColor::grey;    // default colors - can be changed.
	ofColor specularColor = ofColor::lightGray;
};

//  General purpose sphere  (assume parametric)
//
class Sphere: public SceneObject {
public:
	Sphere(glm::vec3 p, float r, ofColor diffuse = ofColor::lightGray) { position = p; radius = r; diffuseColor = diffuse; }
	Sphere() {}
	bool intersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) {
		return (glm::intersectRaySphere(ray.p, ray.d, position, radius, point, normal));
	}
	void draw()  {
		ofSetColor(diffuseColor);

		ofDrawSphere(position, radius); 
	}
	void applyTexture(glm::vec3 appPt) {}
	void loadTexture(ofImage aTexture) { texture = aTexture; }

	float radius = 1.0;
};

// TRIANGLE CLASS IS PART OF FINAL PROJECT: 
// PART OF TOPIC 1: RENDERING A TRIANGLE MESH AND SMOOTH SHADING
// Triangle class needed to store the indeces of the vertices that make up
// a triangle in the mesh from the mesh's list of vertices
// and stores the indeces of the vertex normals of the triangle from
// the mesh's list of vertex normals
// Helper Triangle class for Mesh made up of triangles
// from Project 1
//
class Triangle {
public:
	Triangle(int ind1, int ind2, int ind3, int indN1 = 0, int indN2 = 0, int indN3 = 0) {
		// indices of triangle vertex position
		indVerts[0] = ind1;
		indVerts[1] = ind2;
		indVerts[2] = ind3;
		// indices of vertex normals
		indVerts[3] = indN1;
		indVerts[4] = indN2;
		indVerts[5] = indN3;
	}

	int indVerts[6];
};

// Mesh class - COMPLETED FOR FINAL PROJECT
// Mesh class represents a mesh that can be loaded into 
// the scene as a scene object to be rendered into an image
// influenced by Project 1
//
class Mesh : public SceneObject {
public:
	Mesh(glm::vec3 p, ofColor diffuse = ofColor::purple) {
		position = p;
		diffuseColor = diffuse;
		smoothShading = false;
		cout << "Smooth shading turned off." << endl;
		// load a default triangle first
		// helps with debugging mesh rendering and mesh positioning
		vertices.push_back(glm::vec3(-1 + position.x, -1 + position.y, 0 + position.z));
		vertices.push_back(glm::vec3(1 + position.x, -1 + position.y, 0 + position.z));
		vertices.push_back(glm::vec3(0 + position.x, 1 + position.y, -1 + position.z));
		tris.push_back(Triangle(0, 1, 2));
		vNorms.push_back(glm::cross(
			glm::normalize(vertices[tris[0].indVerts[1]] - vertices[tris[0].indVerts[0]]),
			glm::normalize(vertices[tris[0].indVerts[2]] - vertices[tris[0].indVerts[1]])));
	}
	// INTERSECT FUNCTION MAIN PART FOR FINAL PROJECT TOPIC 1
	// RENDERING A TRIANGLE MESH AND SMOOTH SHADING
	// The intersect function is the main function needed
	// to allow for a mesh to be rendered because the ray tracer calls
	// this function to determine if a ray has hit the mesh and the intersected point
	// and normal will be used to determine how a pixel will be shaded. 
	bool intersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) {
		bool isIntersect = false;
		float shortestDist = INFINITY;
		// loop through every triangle in the mesh to detect intersection
		for (Triangle t : tris) {
			glm::vec2 bPos;
			float d;
			// intersectRayTriangle is a glm function needed to check for ray-triangle intersection
			// will return a boolean as well as gives the barycentric coordinates of the intersection point
			bool isThisIntersect = glm::intersectRayTriangle(ray.p, ray.d, vertices[t.indVerts[0]], vertices[t.indVerts[1]], vertices[t.indVerts[2]], bPos, d);
			if (isThisIntersect) {
				isIntersect = true;
				//point = vertices[t.indVerts[0]]
					//+ (bPos.x * (vertices[t.indVerts[1]] - vertices[t.indVerts[0]]))
					//+ (bPos.y * (vertices[t.indVerts[2]] - vertices[t.indVerts[0]]));		
				// point calculation for intersected point in world space
				glm::vec3 hitPoint = ray.p + (glm::normalize(ray.d) * d);
				float theDist = glm::distance(ray.p, hitPoint);
				// necessary check so that only the closer triangle that intersects with the
				// ray is returned
				if (theDist < shortestDist) {
					shortestDist = theDist;
					point = hitPoint;
					//cout << d << endl;
					if (smoothShading) {
						// the normal to return in order to smooth shade the mesh
						normal = ((1 - bPos.x - bPos.y) * vNorms[t.indVerts[3]]) +
							(bPos.x * vNorms[t.indVerts[4]]) +
							(bPos.y * vNorms[t.indVerts[5]]);
					} else {
						// the normal to return for flat shading the mesh
						normal = glm::cross(
							glm::normalize(vertices[t.indVerts[1]] - vertices[t.indVerts[0]]),
							glm::normalize(vertices[t.indVerts[2]] - vertices[t.indVerts[1]]));
					}
					//normal = glm::vec3(0, 0, 1); 
				}
			}
		}
		return isIntersect;
	}
	void draw() { 
		for (Triangle t : tris) {
			ofSetColor(diffuseColor);
			ofNoFill();
			ofDrawTriangle(vertices[t.indVerts[0]], 
				vertices[t.indVerts[1]], vertices[t.indVerts[2]]);
			ofFill();
		}
	}
	void addMesh(string objFile);
	void applyTexture(glm::vec3 appPt) {}
	void loadTexture(ofImage aTexture) { texture = aTexture; }
	// call this method to toggle smooth shading of the mesh
	void toggleSmooth() { 
		smoothShading = !smoothShading; 
		if (smoothShading) {
			cout << "Smooth shading turned on." << endl;
		} else {
			cout << "Smooth shading turned off." << endl;
		}
	}

	bool smoothShading;
	vector<glm::vec3> vertices;
	vector<glm::vec3> vNorms;
	vector<Triangle> tris;
};


//  General purpose plane 
//
class Plane: public SceneObject {
public:
	Plane(glm::vec3 p, glm::vec3 n, ofColor diffuse = ofColor::darkOliveGreen, float w = 20, float h = 20) {
		position = p; normal = n;
		width = w;
		height = h;
		diffuseColor = diffuse;
		if (normal == glm::vec3(0, 1, 0)) { 
			plane.rotateDeg(90, 1, 0, 0); 
			origin = glm::vec3(position.x - (width / 2.0), position.y, position.z - (height / 2.0));
			uv = glm::vec3(position.x + (width / 2.0), position.y, position.z + (height / 2.0));
		}
		/*
		cout << position << endl;
		cout << origin << endl;
		cout << uv << endl;
		*/
	}
	Plane() { 
		normal = glm::vec3(0, 1, 0);
		plane.rotateDeg(90, 1, 0, 0);
	}
	
	bool intersect(const Ray &ray, glm::vec3 & point, glm::vec3 & normal);
	float sdf(const glm::vec3 &p);
	glm::vec3 getNormal(const glm::vec3 &p) { return this->normal; }
	void draw() {
		ofSetColor(diffuseColor);

		plane.setPosition(position);
		plane.setWidth(width);
		plane.setHeight(height);
		plane.setResolution(4, 4);
		plane.drawWireframe();
	}
	// find world corrdinate from uv coordinate (only use for y normal plane)
	glm::vec3 yNormUVtoWorld(float theU, float theV) {
		float moveU = theU * width;
		float moveV = theV * height;
		return (glm::vec3(moveU + origin.x, position.y, moveV + origin.z));
	}
	// find uv coordinate from world coordinate (only use for y normal plane)
	glm::vec2 yNormWorldtoUV(glm::vec3 planePt) {
		float xDiff = planePt.x - origin.x;
		float zDiff = planePt.z - origin.z;
		return glm::vec2(xDiff / width, zDiff / height);
	}
	void applyTexture(glm::vec3 appPt);
	void loadTexture(ofImage aTexture) { texture = aTexture; }
	// change number of tiles displayed on the floor
	void setNumTiles(int newNumTiles) {
		numTiles = newNumTiles;
	}

	ofPlanePrimitive plane;
	glm::vec3 normal;

	float width = 20;
	float height = 20;
	glm::vec3 origin;
	glm::vec3 uv;
	int numTiles = 10;
};

// view plane for render camera
// 
class  ViewPlane: public Plane {
public:
	ViewPlane(glm::vec2 p0, glm::vec2 p1) { min = p0; max = p1; }

	ViewPlane() {                         // create reasonable defaults (6x4 aspect)
		min = glm::vec2(-3, -2);
		max = glm::vec2(3, 2);
		position = glm::vec3(0, 0, 5);
		normal = glm::vec3(0, 0, 1);      // viewplane currently limited to Z axis orientation
	}

	void setSize(glm::vec2 min, glm::vec2 max) { this->min = min; this->max = max; }
	float getAspect() { return width() / height(); }

	glm::vec3 toWorld(float u, float v);   //   (u, v) --> (x, y, z) [ world space ]

	void draw() {
		ofDrawRectangle(glm::vec3(min.x, min.y, position.z), width(), height());
	}

	
	float width() {
		return (max.x - min.x);
	}
	float height() {
		return (max.y - min.y); 
	}

	// some convenience methods for returning the corners
	//
	glm::vec2 topLeft() { return glm::vec2(min.x, max.y); }
	glm::vec2 topRight() { return max; }
	glm::vec2 bottomLeft() { return min;  }
	glm::vec2 bottomRight() { return glm::vec2(max.x, min.y); }

	//  To define an infinite plane, we just need a point and normal.
	//  The ViewPlane is a finite plane so we need to define the boundaries.
	//  We will define this in terms of min, max  in 2D.  
	//  (in local 2D space of the plane)
	//  ultimately, will want to locate the ViewPlane with RenderCam anywhere
	//  in the scene, so it is easier to define the View rectangle in a local'
	//  coordinate system.
	//
	glm::vec2 min, max;
};


//  render camera  - currently must be z axis aligned (we will improve this in project 4)
//
class RenderCam: public SceneObject {
public:
	RenderCam() {
		position = glm::vec3(0, 0, 10);
		aim = glm::vec3(0, 0, -1);
	}
	Ray getRay(float u, float v);
	void draw() { ofDrawBox(position, 1.0); };
	void drawFrustum();
	void applyTexture(glm::vec3 appPt) {}
	void loadTexture(ofImage aTexture) { }

	glm::vec3 aim;
	ViewPlane view;          // The camera viewplane, this is the view that we will render 
};

// Light class to represent lighting in the environment for shading
//
class Light : public SceneObject {
public:
	Light(glm::vec3 pos, float i) {
		this->position = pos;
		this->intensity = i;
	}

	void draw() {
		ofDrawSphere(position, 0.1);
	}

	bool intersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) { 
		return false; 
	}

	void changeIntensity(float i) {
		intensity = i;
	}
	void applyTexture(glm::vec3 appPt) {}
	void loadTexture(ofImage aTexture) { }

	float intensity;
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void rayTrace();
		void drawGrid();
		void drawAxis(glm::vec3 position);

		ofColor lambert(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, SceneObject* s);
		ofColor phong(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, const ofColor specular, float power, SceneObject* s);
	
		ofxPanel gui;
		ofxFloatSlider lightIntensity;
		ofxFloatSlider lightIntensity2;
		ofxFloatSlider lightIntensity3;
		ofxIntSlider powerExp;
		ofxIntSlider tiles;
		ofxIntSlider gridsAA;

		bool bHide = true;
		bool bShowGui = true;

		ofEasyCam  mainCam;
		ofCamera sideCam;
		ofCamera previewCam;
		ofCamera  *theCam;    // set to current camera either mainCam or sideCam

		// set up one render camera to render image throughn
		//
		RenderCam renderCam;
		ofImage image;
		ofImage floorTexture;

		bool imageLoaded = false;

		bool textureLoaded = false;
		int floorTextureNum = 1;

		Mesh* aMesh;
		Plane* floorPlane;
		vector<SceneObject *> scene;
		vector <Light *> lights;

		bool antiAliasing = false;

		int imageWidth = 1200;
		int imageHeight = 800;
};
 