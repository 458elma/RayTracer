#include "ofApp.h"

/*

	Elston Ma
	CS 116A

*/

// Intersect Ray with Plane  (wrapper on glm::intersect*
//
bool Plane::intersect(const Ray &ray, glm::vec3 & point, glm::vec3 & normalAtIntersect) {
	float dist;
	bool insidePlane = false;
	bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
	if (hit) {
		Ray r = ray;
		point = r.evalPoint(dist);
		normalAtIntersect = this->normal;
		glm::vec2 xrange = glm::vec2(position.x - width / 2,
			position.x + width / 2);
		glm::vec2 zrange = glm::vec2(position.z - height / 2,
			position.z + height / 2);
		if (point.x < xrange[1] && point.x > xrange[0] && 
			point.z < zrange[1] && point.z > zrange[0]) {
			insidePlane = true;
		}
	}
	return insidePlane;
}


// Convert (u, v) to (x, y, z) 
// We assume u,v is in [0, 1]
//
glm::vec3 ViewPlane::toWorld(float u, float v) {
	float w = width();
	float h = height();
	return (glm::vec3((u * w) + min.x, (v * h) + min.y, position.z));
}

// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
//
Ray RenderCam::getRay(float u, float v) {
	glm::vec3 pointOnPlane = view.toWorld(u, v);
	return(Ray(position, glm::normalize(pointOnPlane - position)));
}

// draw frustum of render cam
void RenderCam::drawFrustum() {
	ofDrawLine(position, glm::vec3(view.bottomLeft(), view.position.z));
	ofDrawLine(position, glm::vec3(view.bottomRight(), view.position.z));
	ofDrawLine(position, glm::vec3(view.topLeft(), view.position.z));
	ofDrawLine(position, glm::vec3(view.topRight(), view.position.z));
}

// apply texture to a plane (so far only good for y normal plane)
void Plane::applyTexture(glm::vec3 appPt) {
	if (texture.isAllocated()) {
		glm::vec2 uvPt = yNormWorldtoUV(appPt);
		float tempU = numTiles * uvPt.x;
		float tempV = numTiles * uvPt.y;
		float i = (tempU * texture.getWidth()) - 0.5;
		float j = (tempV * texture.getHeight()) - 0.5;
		float newI = fmod(i, texture.getWidth());
		float newJ = fmod(j, texture.getHeight());
		diffuseColor = texture.getColor(newI , newJ);
	}
}

// USED FOR FINAL PROJECT TOPIC 1
// RENDERING A TRIANGLE MESH AND SMOOTH SHADING
// add obj file mesh to Mesh instance
// so that mesh can be drawn into scene
// file reader method for obj files
// from Project 1
void Mesh::addMesh(string objFile) {
	ifstream obj;
	obj.open(objFile);
	if (!obj) {
		ofExit();
	}

	// clear all the lists to replace old mesh
	vertices.clear();
	tris.clear();
	vNorms.clear();

	string line;
	float p1, p2, p3;
	float n1, n2, n3;
	string s1, s2, s3;

	while (!obj.eof()) {
		obj >> line;
		if (!obj.eof()) {
			// load vertices into the vertex vector
			if (line == "v") {
				obj >> p1 >> p2 >> p3;
				vertices.push_back(glm::vec3(p1 + position.x, 
					p2 + position.y, p3 + position.z));
			}

			// load vertex normals into the vertex normal vector
			if (line == "vn") {
				obj >> n1 >> n2 >> n3;
				vNorms.push_back(glm::normalize(glm::vec3(n1, n2, n3)));
			}

			// load triangle faces into the Triangle vector
			// includes both vertex and vertex normal vector indices
			if (line == "f") {
				obj >> s1 >> s2 >> s3;
				int one = stoi(s1.substr(0, s1.find("/"))) - 1;
				int two = stoi(s2.substr(0, s2.find("/"))) - 1;
				int three = stoi(s3.substr(0, s3.find("/"))) - 1;

				string n1t = s1.substr(s1.find("/") + 1);
				string n2t = s2.substr(s2.find("/") + 1);
				string n3t = s3.substr(s3.find("/") + 1);

				int oneN = stoi(n1t.substr(n1t.find("/") + 1)) - 1;
				int twoN = stoi(n2t.substr(n2t.find("/") + 1)) - 1;
				int threeN = stoi(n3t.substr(n3t.find("/") + 1)) - 1;
				//cout << oneN << ", " << twoN << ", " << threeN << endl;

				tris.push_back(Triangle(one, two, three, oneN, twoN, threeN));
			}
		}
	}

	obj.close();
}

// ray tracer algorithm
void ofApp::rayTrace() {
	cout << "rendering..." << endl;

	image.allocate(imageWidth, imageHeight, OF_IMAGE_COLOR);

	// loop through every pixel of the image to print
	for (int y = 0; y < imageHeight; y++) {
		for (int x = 0; x < imageWidth; x++) {
			/*
			// test for image output
			if ((x < (imageWidth / 2.0)) && 
				(y < (imageHeight / 2.0))) {
				image.setColor(x, y, ofColor::blue);
			}
			if ((x >= (imageWidth / 2.0)) &&
				(y >= (imageHeight / 2.0))) {
				image.setColor(x, y, ofColor::red);
			}
			if ((x < (imageWidth / 2.0)) &&
				(y > (imageHeight / 2.0))) {
				image.setColor(x, y, ofColor::green);
			}
			*/
			if (!antiAliasing) {
				// the ray to detect intersections
				Ray toShoot = renderCam.getRay(((x + 0.5) / imageWidth), ((y + 0.5) / imageHeight));

				//cout << (float)(x / imageWidth) << ", " << (float)(y / imageHeight) << endl;

				glm::vec3 hitPt;
				glm::vec3 hitNorm;
				bool hit = false; // used to indicate if any intersection happened
				float minDist = INFINITY;
				SceneObject* pixelObj = NULL;
				// loop through every object in the scene to detect intersection
				for (SceneObject* sObj : scene) {
					hit = hit || sObj->intersect(toShoot, hitPt, hitNorm);

					// if there is an intersection and the object hit is closest to the render cam
					// then color the image pixel the color of the object hit
					if (sObj->intersect(toShoot, hitPt, hitNorm)) {
						if (glm::distance(renderCam.position, hitPt) < minDist) {
							minDist = glm::distance(renderCam.position, hitPt);
							//image.setColor(x, imageHeight - y - 1, sObj->diffuseColor);
							pixelObj = sObj;
						}
					}
				}

				// if there is no intersection, set pixel to black
				if (!hit) {
					image.setColor(x, imageHeight - y - 1, ofColor::black);
				}
				else {
					pixelObj->intersect(toShoot, hitPt, hitNorm);

					pixelObj->applyTexture(hitPt);

					//ofColor newColor = lambert(hitPt, glm::normalize(hitNorm), pixelObj->diffuseColor);

					ofColor newColor = phong(hitPt, glm::normalize(hitNorm),
						pixelObj->diffuseColor, pixelObj->specularColor, powerExp, pixelObj);

					image.setColor(x, imageHeight - y - 1, newColor);
				}
			} else {
				// HERE IS FINAL PROJECT TOPIC 2
				// THIS IS WHERE THE ANTI-ALIASING ALGORITHM IS

				// the oversampling rays to detect intersections for anti-aliasing
				vector<Ray> sRays;
				float gridSize = 1.0f / gridsAA;
				//cout << gridSize << endl;

				// create rays to shoot through random spots of a specific grid of the pixel
				// pixel divided into number of grids determined by application slider
				for (float i = 0; i < 1; i += gridSize) {
					for (float j = 0; j < 1; j += gridSize) {
						sRays.push_back(renderCam.getRay(((x + ofRandom(i, i + gridSize)) / imageWidth), ((y + ofRandom(j, j + gridSize)) / imageHeight)));
					}
				}
				/*
				for (float i = 0.1f; i < 1; i += 0.2) {
					for (float j = 0.1f; j < 1; j += 0.2) {
						sRays.push_back(renderCam.getRay(((x + i) / imageWidth), ((y + j) / imageHeight)));
					}
				}
				*/

				vector<ofFloatColor> colors; // colors to average later

				//cout << (float)(x / imageWidth) << ", " << (float)(y / imageHeight) << endl;
				// loop through every ray, every ray does standard ray tracing algorithm like the one above
				// the difference is it will push the shaded color into the colors vector
				for (Ray toShoot : sRays) {
					glm::vec3 hitPt;
					glm::vec3 hitNorm;
					bool hit = false; // used to indicate if any intersection happened
					float minDist = INFINITY;
					SceneObject* pixelObj = NULL;
					// loop through every object in the scene to detect intersection
					for (SceneObject* sObj : scene) {
						hit = hit || sObj->intersect(toShoot, hitPt, hitNorm);

						// if there is an intersection and the object hit is closest to the render cam
						// then color the image pixel the color of the object hit
						if (sObj->intersect(toShoot, hitPt, hitNorm)) {
							if (glm::distance(renderCam.position, hitPt) < minDist) {
								minDist = glm::distance(renderCam.position, hitPt);
								//image.setColor(x, imageHeight - y - 1, sObj->diffuseColor);
								pixelObj = sObj;
							}
						}
					}

					// if there is no intersection, push color black to color vector
					if (!hit) {
						//image.setColor(x, imageHeight - y - 1, ofColor::black);
						colors.push_back(ofFloatColor::black);
					}
					else {
						pixelObj->intersect(toShoot, hitPt, hitNorm);

						pixelObj->applyTexture(hitPt);

						//ofColor newColor = lambert(hitPt, glm::normalize(hitNorm), pixelObj->diffuseColor);

						ofFloatColor newColor = phong(hitPt, glm::normalize(hitNorm),
							pixelObj->diffuseColor, pixelObj->specularColor, powerExp, pixelObj);

						//image.setColor(x, imageHeight - y - 1, newColor);
						//colors.push_back(pixelObj->diffuseColor);
						colors.push_back(newColor); // push shaded color into the colors vector
					}
				}

				// average out the colors detected in this pixel to show anti-aliasing effect
				ofFloatColor totalColor = 0;
				for (ofFloatColor c : colors) {
					totalColor = totalColor + (c / (float)colors.size());
				}
				image.setColor(x, imageHeight - y - 1, totalColor);
			}
		}
	}

	image.save("image/renderedScene.png");

	image.load("image/renderedScene.png");
	imageLoaded = true;

	cout << "done" << endl;
}

// lambert shading algorithm
ofColor ofApp::lambert(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, SceneObject* s) {
	float ambientInt = 0.07;

	ofColor ld = 0;
	// loop through all lights to apply proper diffuse shading
	for (Light* light : lights) {
		Ray shadowRay(p + (0.0001 * norm), glm::normalize(light->position - p));
		glm::vec3 pt;
		glm::vec3 no;
		bool shadHit = false;
		// loop through scene objcets to see if point is blocked from light source
		// no shadow casted on meshes
		Mesh* mPtr = dynamic_cast<Mesh*>(s);
		if (mPtr == nullptr) {
			for (SceneObject* sObj : scene) {
				if (sObj->intersect(shadowRay, pt, no)) {
					shadHit = true;
					break;
				}
			}
		}

		float distance = glm::distance(p, light->position);
		glm::vec3 l = glm::normalize(light->position - p);

		if (!shadHit) 
			ld = ld + (diffuse * (light->intensity / (distance * distance)) * glm::max((float)0, glm::dot(norm, l)));
	}
	ld = ld + (ambientInt * diffuse); // apply ambient shading here

	return ld;
}

// phong shading algorithm
ofColor ofApp::phong(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, const ofColor specular, float power, SceneObject* s) {
	glm::vec3 v = glm::normalize(renderCam.position - p);

	ofColor ls = 0;
	// loop through all lights to apply proper specular shading
	for (Light* light : lights) {
		Ray shadowRay(p + (0.0001 * norm), glm::normalize(light->position - p));
		glm::vec3 pt;
		glm::vec3 no;
		bool shadHit = false;
		// loop through scene objcets to see if point is blocked from light source
		// no shadow casted on meshes
		Mesh* mPtr = dynamic_cast<Mesh*>(s);
		if (mPtr == nullptr) {
			for (SceneObject* sObj : scene) {
				if (sObj->intersect(shadowRay, pt, no)) {
					shadHit = true;
					break;
				}
			}
		}

		float distance = glm::distance(p, light->position);
		glm::vec3 l = glm::normalize(light->position - p);
		glm::vec3 h = glm::normalize(v + l);

		if (!shadHit) 
			ls = ls + (specular * (light->intensity / (distance * distance)) * glm::pow(glm::max((float)0, glm::dot(norm, h)), power));
	}
	ls = ls + lambert(p, norm, diffuse, s); // add the lambert shading done in the lambert function to get full shading

	return ls;
}

//--------------------------------------------------------------
void ofApp::setup(){
	ofEnableDepthTest();

	ofSetBackgroundColor(ofColor::black);
	theCam = &mainCam;
	mainCam.setDistance(10);
	mainCam.setNearClip(0.1);
	previewCam.setPosition(renderCam.position);
	previewCam.lookAt(glm::vec3(0, 0, 0));
	previewCam.setNearClip(0.1);
	sideCam.setPosition(glm::vec3(10, 0, 0));
	sideCam.lookAt(glm::vec3(0, 0, 0));
	sideCam.setNearClip(0.1);

	// load floor texture image
	textureLoaded = floorTexture.load("textures/floralWoodTexture.jpg");
	//textureLoaded = floorTexture.load("textures/mosaicTexture.jpg");
	//textureLoaded = floorTexture.load("textures/stoneTexture.jpg");
	//textureLoaded = floorTexture.load("textures/concreteTileTexture.jpg");
	//textureLoaded = floorTexture.load("textures/marbleTileTexture.png");

	//cout << textureLoaded << endl;
	if (!textureLoaded) {
		ofExit();
	}
	cout << "floralWoodTexture loaded" << endl;
	cout << "Anti-aliasing turned off." << endl;

	// setup scene to render (three spheres on a plane)
	float moveZ = -7;
	scene.push_back(new Sphere(glm::vec3(-1, 0, 0 + moveZ), 2.0, ofColor::red));
	scene.push_back(new Sphere(glm::vec3(1, 0, -4 + moveZ), 2.0, ofColor::teal));
	scene.push_back(new Sphere(glm::vec3(0, 0, 2 + moveZ), 1.0, ofColor::yellow));
	floorPlane = new Plane(glm::vec3(0, -2, 0 + moveZ), glm::vec3(0, 1, 0), ofColor::purple);
	floorPlane->loadTexture(floorTexture);
	scene.push_back(floorPlane);
	float moveX = -7;
	scene.push_back(new Sphere(glm::vec3(10.5 + moveX, -1.25, 5 + moveZ), 0.5, ofColor::aquamarine));
	scene.push_back(new Sphere(glm::vec3(10.5 + moveX, -0.55, 5 + moveZ), 0.4, ofColor::gold));
	scene.push_back(new Sphere(glm::vec3(10.5 + moveX, 0.05, 5 + moveZ), 0.3, ofColor::pink));
	scene.push_back(new Mesh(glm::vec3(-7, 0, -9), ofColor::green));
	aMesh = dynamic_cast<Mesh*>(scene[scene.size() - 1]);

	// setup sliders
	gui.setup();
	gui.add(lightIntensity.setup("Top Light Intensity", 25, 0, 500));
	gui.add(lightIntensity2.setup("Left Light Intensity", 25, 0, 500));
	gui.add(lightIntensity3.setup("Front Light Intensity", 25, 0, 500));
	gui.add(powerExp.setup("Phong Exponent", 10, 10, 10000));
	gui.add(tiles.setup("Floor Tiles", 10, 1, 15));
	gui.add(gridsAA.setup("Anti-aliasing dimension", 5, 2, 10));

	// setup lights
	lights.push_back(new Light(glm::vec3(2.5, 8, 0.5 + moveZ), lightIntensity));
	lights.push_back(new Light(glm::vec3(-11, 5, 2 + moveZ), lightIntensity2));
	lights.push_back(new Light(glm::vec3(-11, 0, 8 + moveZ), lightIntensity3));
	//lights.push_back(new Light(glm::vec3(0, 0, -7), lightIntensity));

}

//--------------------------------------------------------------
void ofApp::update(){
	// update light intensity according to slider values
	lights[0]->changeIntensity(lightIntensity);
	lights[1]->changeIntensity(lightIntensity2);
	lights[2]->changeIntensity(lightIntensity3);
	// update number of floor tiles to use
	floorPlane->setNumTiles(tiles);
}

//--------------------------------------------------------------
void ofApp::draw(){
	theCam->begin();

	ofFill();
	for (SceneObject* sObj : scene) {
		sObj->draw();
	}

	for (Light* l: lights) {
		l->draw();
	}

	ofNoFill();
	renderCam.draw();
	renderCam.view.draw();
	renderCam.drawFrustum();

	theCam->end();

	// draw the rendered image if option selected
	// and image is rendered and loaded
	if (!bHide && imageLoaded) {
		ofSetColor(ofColor::white);

		// maintain aspect ratio of display image but also fits the window
		float w = image.getWidth();
		float h = image.getHeight();
		while (w > ofGetWindowWidth() || h > ofGetWindowHeight()) {
			w = w * 0.9;
			h = h * 0.9;
		}
		//cout << w << ", " << h << endl;

		// center the image on the window
		float wCenterW = ofGetWindowWidth() / 2.0;
		float wCenterH = ofGetWindowHeight() / 2.0;
		float iCenterW = w / 2;
		float iCenterH = h / 2;
		image.draw(wCenterW - iCenterW, wCenterH - iCenterH, w, h);
	}

	ofDisableDepthTest();
	if (bShowGui) gui.draw();
	ofEnableDepthTest();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
	// press 1 to use easy cam
	case '1':
		//cout << "1 pressed" << endl;
		mainCam.enableMouseInput();
		theCam = &mainCam;
		break;
	// press 2 to see side cam view
	case '2':
		mainCam.disableMouseInput();
		theCam = &sideCam;
		break;
	// press 3 to preview render view
	case '3':
		mainCam.disableMouseInput();
		theCam = &previewCam;
		break;
	// ADDED FOR FINAL PROJECT
	// press a to toggle antialiasing
	case 'A':
	case 'a':
		antiAliasing = !antiAliasing;
		if (antiAliasing) {
			cout << "Anti-aliasing turned on." << endl;
		} else {
			cout << "Anti-aliasing turned off." << endl;
		}
		break;
	// ADDED FOR FINAL PROJECT
	// press s to toggle smooth shading for the mesh
	case 'S':
	case 's':
		if (aMesh != nullptr) {
			aMesh->toggleSmooth();
		}
		break;
	// press r to render the scene into an image
	case 'R':
	case 'r':
		rayTrace();
		break;
	// press i to show rendered scene
	case 'I':
	case 'i':
		bHide = !bHide;
		break;
	// press h to toggle gui visibility
	case 'H':
	case 'h':
		bShowGui = !bShowGui;
		break;
	// press f to toggle floor texture to use
	case 'F':
	case 'f':
		switch (floorTextureNum) {
		case 0:
			textureLoaded = floorTexture.load("textures/floralWoodTexture.jpg");
			if (!textureLoaded) {
				ofExit();
			}
			cout << "floralWoodTexture loaded" << endl;
			floorPlane->loadTexture(floorTexture);
			floorTextureNum++;
			break;
		case 1:
			textureLoaded = floorTexture.load("textures/mosaicTexture.jpg");
			if (!textureLoaded) {
				ofExit();
			}
			cout << "mosaicTexture loaded" << endl;
			floorPlane->loadTexture(floorTexture);
			floorTextureNum++;
			break;
		case 2:
			textureLoaded = floorTexture.load("textures/stoneTexture.jpg");
			if (!textureLoaded) {
				ofExit();
			}
			cout << "stoneTexture loaded" << endl;
			floorPlane->loadTexture(floorTexture);
			floorTextureNum++;
			break;
		case 3:
			textureLoaded = floorTexture.load("textures/concreteTileTexture.jpg");
			if (!textureLoaded) {
				ofExit();
			}
			cout << "concreteTileTexture loaded" << endl;
			floorPlane->loadTexture(floorTexture);
			floorTextureNum++;
			break;
		case 4:
			textureLoaded = floorTexture.load("textures/marbleTileTexture.png");
			if (!textureLoaded) {
				ofExit();
			}
			cout << "marbleTileTexture loaded" << endl;
			floorPlane->loadTexture(floorTexture);
			floorTextureNum = 0;
			break;
		default:
			textureLoaded = floorTexture.load("textures/floralWoodTexture.jpg");
			if (!textureLoaded) {
				ofExit();
			}
			cout << "floralWoodTexture loaded" << endl;
			floorPlane->loadTexture(floorTexture);
			floorTextureNum = 0;
		}
		break;
	}

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
	// ADDED FOR FINAL PROJECT 
	// used to read in a dragged obj file
	// the obj file will be the mesh to render
	Mesh* theMesh = dynamic_cast<Mesh*>(scene[scene.size() - 1]);
	if (theMesh != nullptr) {
		theMesh->addMesh(dragInfo.files[0]);
	}
}
