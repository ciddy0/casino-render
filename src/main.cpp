/**
This application displays a mesh in wireframe using "Modern" OpenGL 3.0+.
The Mesh3D class now initializes a "vertex array" on the GPU to store the vertices
	and faces of the mesh. To render, the Mesh3D object simply triggers the GPU to draw
	the stored mesh data.
We now transform local space vertices to clip space using uniform matrices in the vertex shader.
	See "simple_perspective.vert" for a vertex shader that uses uniform model, view, and projection
		matrices to transform to clip space.
	See "uniform_color.frag" for a fragment shader that sets a pixel to a uniform parameter.
*/
#define _USE_MATH_DEFINES
#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <filesystem>
#include <math.h>

#include "AssimpImport.h"
#include "Mesh3D.h"
#include "Object3D.h"
#include "Animator.h"
#include "ShaderProgram.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "PauseAnimation.h"
#include "BezierAnimation.h"
struct Scene {
	ShaderProgram program;
	std::vector<Object3D> objects;
	std::vector<Animator> animators;
};

/**
 * @brief Constructs a shader program that applies the Phong reflection model.
 */
ShaderProgram phongLightingShader() {
	ShaderProgram shader;
	try {
		// These shaders are INCOMPLETE.
		shader.load("shaders/light_perspective.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Constructs a shader program that performs texture mapping with no lighting.
 */
ShaderProgram texturingShader() {
	ShaderProgram shader;
	try {
		shader.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Loads an image from the given path into an OpenGL texture.
 */
Texture loadTexture(const std::filesystem::path& path, const std::string& samplerName = "baseTexture") {
	StbImage i;
	i.loadFromFile(path.string());
	return Texture::loadImage(i, samplerName);
}

/*****************************************************************************************
*  DEMONSTRATION SCENES
*****************************************************************************************/
Scene bunny() {
	Scene scene{ texturingShader() };

	// We assume that (0,0) in texture space is the upper left corner, but some artists use (0,0) in the lower
	// left corner. In that case, we have to flip the V-coordinate of each UV texture location. The last parameter
	// to assimpLoad controls this. If you load a model and it looks very strange, try changing the last parameter.
	auto bunny = assimpLoad("models/bunny_textured.obj", true);
	bunny.grow(glm::vec3(9, 9, 9));
	bunny.move(glm::vec3(0.2, -1, 0));
	
	// Move all objects into the scene's objects list.
	scene.objects.push_back(std::move(bunny));
	// Now the "bunny" variable is empty; if we want to refer to the bunny object, we need to reference 
	// scene.objects[0]

	Animator spinBunny;
	// Spin the bunny 360 degrees over 10 seconds.
	spinBunny.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	
	// Move all animators into the scene's animators list.
	scene.animators.push_back(std::move(spinBunny));

	return scene;
}


/**
 * @brief Demonstrates loading a square, oriented as the "floor", with a manually-specified texture
 * that does not come from Assimp.
 */
Scene marbleSquare() {
	Scene scene{ texturingShader() };

	std::vector<Texture> textures = {
		loadTexture("models/White_marble_03/Textures_2K/white_marble_03_2k_baseColor.tga", "baseTexture"),
	};
	auto mesh = Mesh3D::square(textures);
	auto floor = Object3D(std::vector<Mesh3D>{mesh});
	floor.grow(glm::vec3(5, 5, 5));
	floor.move(glm::vec3(0, -1.5, 0));
	floor.rotate(glm::vec3(-M_PI / 2, 0, 0));

	scene.objects.push_back(std::move(floor));
	return scene;
}

/**
 * @brief Loads a cube with a cube map texture.
 */
Scene cube() {
	Scene scene{ texturingShader() };

	auto cube = assimpLoad("models/cube.obj", true);

	scene.objects.push_back(std::move(cube));

	Animator spinCube;
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	// Then spin around the x axis.
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(2 * M_PI, 0, 0)));

	scene.animators.push_back(std::move(spinCube));

	return scene;
}

/**
 * @brief Constructs a scene of a tiger sitting in a boat, where the tiger is the child object
 * of the boat.
 * @return
 */
Scene lifeOfPi() {
	// This scene is more complicated; it has child objects, as well as animators.
	Scene scene{ texturingShader() };

	auto boat = assimpLoad("models/boat/boat.fbx", true);
	boat.move(glm::vec3(0, -0.7, 0));
	boat.grow(glm::vec3(0.01, 0.01, 0.01));
	auto tiger = assimpLoad("models/tiger/scene.gltf", true);
	tiger.move(glm::vec3(0, -5, 10));
	// Move the tiger to be a child of the boat.
	boat.addChild(std::move(tiger));

	// Move the boat into the scene list.
	scene.objects.push_back(std::move(boat));

	// We want these animations to referenced the *moved* objects, which are no longer
	// in the variables named "tiger" and "boat". "boat" is now in the "objects" list at
	// index 0, and "tiger" is the index-1 child of the boat.
	Animator animBoat;
	animBoat.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10, glm::vec3(0, 2 * M_PI, 0)));
	Animator animTiger;
	animTiger.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0].getChild(1), 10, glm::vec3(0, 0, 2 * M_PI)));

	// The Animators will be destroyed when leaving this function, so we move them into
	// a list to be returned.
	scene.animators.push_back(std::move(animBoat));
	scene.animators.push_back(std::move(animTiger));

	// Transfer ownership of the objects and animators back to the main.
	return scene;
}

Scene Casino() {
	Scene scene{ texturingShader()
	};
	std::vector<Texture> floorTextures = {
		loadTexture("models/carpet.jpeg", "baseTexture"),
	};
	auto floorMesh = Mesh3D::square(floorTextures);
	auto floor = Object3D(std::vector<Mesh3D>{floorMesh});
	floor.grow(glm::vec3(5, 5, 5));
	floor.move(glm::vec3(0, 0, 0));
	floor.rotate(glm::vec3(-M_PI / 2, 0, 0));
	// 0
	scene.objects.push_back(std::move(floor));
	// slot machine
	auto slots = assimpLoad("models/slot_machine/scene.gltf", true);
	slots.setScale(glm::vec3(1));
	slots.setPosition(glm::vec3(-1, 0, -3));
	// 1
	scene.objects.push_back(std::move(slots));
	std::cout << slots.isMoving << std::endl;

	// table
	auto table = assimpLoad("models/poker_table/scene.gltf", true);
	table.setScale(glm::vec3(.001));
	table.setPosition(glm::vec3(0, 0, 0));
	std::cout << table.isMoving << std::endl;
	// 2
	scene.objects.push_back(std::move(table));

	// cards and chips
	auto casinoChips = assimpLoad("models/casino_chips/scene.gltf", true);
	casinoChips.setScale(glm::vec3(1));
	casinoChips.setPosition(glm::vec3(.4, .6, 0));
	std::cout << casinoChips.isMoving << std::endl;
	// 3
	scene.objects.push_back(std::move(casinoChips));

	// animated slot (ugly)
	auto slots2 = assimpLoad("models/slotmachine3/scene.gltf", true);
	slots2.setScale(glm::vec3(2));
	slots2.setPosition(glm::vec3(0, 1, -2));
	slots2.rotate(glm::vec3(0, -M_PI/2, 0));
	Animator animLever;
	Animator animLeverUp;
	Animator animWheel1;
	Animator animWheel2;
	Animator animWheel3;
	// 4
	scene.objects.push_back(std::move(slots2));
	animLever.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(1).getChild(0), 1, glm::vec3(0, 0, .5*(-2*M_PI))));
	animLever.addAnimation(std::make_unique<PauseAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(1).getChild(0), .5));
	animLever.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(1).getChild(0), 1, glm::vec3(0, 0, .5*(2*M_PI))));


	animWheel1.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(2).getChild(0), 3, glm::vec3(0, 10*(-2*M_PI), 0)));
	animWheel2.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(3).getChild(0),5 , glm::vec3(0, 2*(-2*M_PI), 0)));
	animWheel3.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(4).getChild(0), 7, glm::vec3(0, -2*M_PI, 0)));

	// cube
	auto cube = assimpLoad("models/dice/scene.gltf", true);
	cube.setScale(glm::vec3(.05));
	cube.move(glm::vec3(0, 2, 0));
	cube.setAcceleration(glm::vec3(0, -9.8, 0));
	cube.setVelocity(glm::vec3(-.5, .5, 0));
	cube.setAngularVelocity(glm::vec3(8, 5, 2));
	cube.setBounceCoeff(0.5);
	cube.isMoving = true;
	// 5
	scene.objects.push_back(std::move(cube));

	// second cube
	auto cube2 = assimpLoad("models/dice/scene.gltf", true);
	cube2.setScale(glm::vec3(.05));
	cube2.move(glm::vec3(-.5, 2, 0));
	cube2.setAcceleration(glm::vec3(0, -9.8, 0));
	cube2.setVelocity(glm::vec3(.5, .5, -.5));
	cube2.setAngularVelocity(glm::vec3(12, 1, 5));
	cube2.setBounceCoeff(0.5);
	cube2.isMoving = true;
	// 6
	scene.objects.push_back(std::move(cube2));

	// letter g
	auto letterG = assimpLoad("models/g_letter/scene.gltf", true);
	letterG.setScale(glm::vec3(.5));
	letterG.move(glm::vec3(-.5, 2, 3));
	// 7
	scene.objects.push_back(std::move(letterG));

	glm::vec3 p0 = glm::vec3(-.5, 2, 3);
	glm::vec3 p1 = glm::vec3(0, .5, 0);
	glm::vec3 p2 = glm::vec3(1, 0, 0);
	glm::vec3 p3_g = glm::vec3(-.5, 2, 0);
	glm::vec3 p3_a = glm::vec3(-.2, 2, 0);
	glm::vec3 p3_t = glm::vec3(0.1, 2, 0);
	glm::vec3 p3_o = glm::vec3(.4, 2, 0);

	//letter a
	auto letterA = assimpLoad("models/a_letter/scene.gltf", true);
	letterA.setScale(glm::vec3(.5));
	letterA.move(glm::vec3(-.2, 2, 3));
	scene.objects.push_back(std::move(letterA));

	// letter t
	auto letterT = assimpLoad("models/t_letter/scene.gltf", true);
	letterT.setScale(glm::vec3(.5));
	letterT.move(glm::vec3(0.1, 2, 3));
	scene.objects.push_back(std::move(letterT));
	// letter o
	auto letterO = assimpLoad("models/o_letter/scene.gltf", true);
	letterO.setScale(glm::vec3(.5));
	letterO.move(glm::vec3(.4, 2, 3));
	scene.objects.push_back(std::move(letterO));

	// left wall
	std::vector<Texture> WallTextures = {
		loadTexture("models/casino_left.jpg", "baseTexture"),
	};
	std::vector<Texture> WallTextures2 = {
		loadTexture("models/whitewall.jpg", "baseTexture"),
	};
	auto leftWallMesh = Mesh3D::square(WallTextures);
	auto leftWall = Object3D(std::vector<Mesh3D>{leftWallMesh});
	leftWall.grow(glm::vec3(5, 5, 5));
	leftWall.move(glm::vec3(-2, 2.5, 0));
	leftWall.rotate(glm::vec3(0, M_PI/2, 0));
	// 0
	scene.objects.push_back(std::move(leftWall));

	auto rightWallMesh = Mesh3D::square(WallTextures);
	auto rightWall = Object3D(std::vector<Mesh3D>{rightWallMesh});
	rightWall.grow(glm::vec3(5, 5, 5));
	rightWall.move(glm::vec3(2, 2.5, 0));
	rightWall.rotate(glm::vec3(0, -M_PI/2, 0));
	scene.objects.push_back(std::move(rightWall));

	auto backWallMesh = Mesh3D::square(WallTextures2);
	auto backWall = Object3D(std::vector<Mesh3D>{backWallMesh});
	backWall.grow(glm::vec3(5, 5.8, 5));
	backWall.move(glm::vec3(0, 2.5, -2.8));
	backWall.rotate(glm::vec3(0, 0, 0));
	scene.objects.push_back(std::move(backWall));

	auto frontWallMesh = Mesh3D::square(WallTextures2);
	auto frontWall = Object3D(std::vector<Mesh3D>{frontWallMesh});
	frontWall.grow(glm::vec3(5, 5.8, 5));
	frontWall.move(glm::vec3(0, 2.5, 2.5));
	frontWall.rotate(glm::vec3(0, M_PI, 0));
	scene.objects.push_back(std::move(frontWall));

	std::vector<Texture> ceilingTextures = {
		loadTexture("models/roof.jpg", "baseTexture"),
	};
	auto ceilingMesh = Mesh3D::square(ceilingTextures);
	auto ceiling = Object3D(std::vector<Mesh3D>{ceilingMesh});
	ceiling.grow(glm::vec3(5, 5, 5));
	ceiling.move(glm::vec3(0, 5, 0));
	ceiling.rotate(glm::vec3(-M_PI / 2, 0, M_PI));
	// 0
	scene.objects.push_back(std::move(ceiling));

	Animator animName;
	animName.addAnimation(std::make_unique<PauseAnimation>(scene.objects[7], 7));
	animName.addAnimation(std::make_unique<BezierAnimation>(scene.objects[7], 3, p0, p1, p2, p3_g));
	animName.addAnimation(std::make_unique<BezierAnimation>(scene.objects[8], 3, p0, p1, p2, p3_a));
	animName.addAnimation(std::make_unique<BezierAnimation>(scene.objects[9], 3, p0, p1, p2, p3_t));
	animName.addAnimation(std::make_unique<BezierAnimation>(scene.objects[10], 3, p0, p1, p2, p3_o));

	scene.animators.push_back(std::move(animName));
	scene.animators.push_back(std::move(animLever));
	scene.animators.push_back(std::move(animWheel1));
	scene.animators.push_back(std::move(animWheel2));
	scene.animators.push_back(std::move(animWheel3));
	return scene;

}
void snapToNearestRotation(Object3D& dice) {
	glm::vec3 rot = dice.getOrientation();

	// Convert to degrees and snap to nearest 90 degrees
	rot = glm::degrees(rot);
	rot.x = round(rot.x / 90) * 90;
	rot.y = round(rot.y / 90) * 90;
	rot.z = round(rot.z / 90) * 90;

	// Convert back to radians
	dice.setOrientation(glm::radians(rot));
}
int main() {
	std::cout << std::filesystem::current_path() << std::endl;
	// Initialize the window and OpenGL.
	sf::ContextSettings settings;
	settings.depthBits = 24; // Request a 24 bits depth buffer
	settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	settings.antialiasingLevel = 2;  // Request 2 levels of antialiasing
	settings.majorVersion = 3;
	settings.minorVersion = 3;
	sf::Window window(sf::VideoMode{ 1200, 800 }, "Modern OpenGL", sf::Style::Resize | sf::Style::Close, settings);

	gladLoadGL();
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);

	// Inintialize scene objects.
	auto myScene = Casino();
	// Activate the shader program.
	myScene.program.activate();
	
	// Ready, set, go!
	bool running = true;
	sf::Clock c;
	auto last = c.getElapsedTime();

	// Start the animators.
	for (auto& anim : myScene.animators) {
		anim.start();
	}
	// booleans for keyboard input
	bool throwDice = false;
	bool startAnimation = false;
	// camera view (from lecture)
	glm::vec3 cameraPos = glm::vec3(0, 1.3, 2);
	glm::vec3 cameraDir = glm::vec3(0, 0, -1);
	glm::mat4 camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
	glm::mat4 perspective = glm::perspective(glm::radians(45.0), static_cast<double>(window.getSize().x) / window.getSize().y, 0.1, 100.0);
	float cameraSpeed = 0;
	while (running) {
		sf::Event ev;
		while (window.pollEvent(ev)) {
			if (ev.type == sf::Event::Closed) {
				running = false;
			}
			if (ev.type == sf::Event::KeyPressed) {
				if (ev.key.code == sf::Keyboard::Space) {
					throwDice = true;
				}
				if (ev.key.code == sf::Keyboard::Return) {
					startAnimation = true;
				}
				// camera movement used the approach from lecture! :D
				if (ev.key.code == sf::Keyboard::A) {
					cameraDir = glm::rotate(cameraDir, cameraSpeed, glm::vec3(0,1,0));
					camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
				}
				if (ev.key.code == sf::Keyboard::D) {
					cameraDir = glm::rotate(cameraDir, -cameraSpeed, glm::vec3(0,1,0));
					camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
				}
				if (ev.key.code == sf::Keyboard::W) {
					cameraDir = glm::rotate(cameraDir, cameraSpeed, glm::vec3(1,0,0));
					camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
				}
				if (ev.key.code == sf::Keyboard::S) {
					cameraDir = glm::rotate(cameraDir, -cameraSpeed, glm::vec3(1,0,0));
					camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
				}
				if (ev.key.code == sf::Keyboard::Up) {
					cameraPos += cameraSpeed * cameraDir;
					camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
				}
				if (ev.key.code == sf::Keyboard::Down) {
					cameraPos -= cameraSpeed * cameraDir;
					camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
				}
			}
		}
		auto now = c.getElapsedTime();
		auto diff = now - last;
		// std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;
		last = now;
		cameraSpeed = 100.0f * diff.asSeconds();

		myScene.program.setUniform("view", camera);
		myScene.program.setUniform("projection", perspective);
		myScene.program.setUniform("cameraPos", cameraPos);

		if (startAnimation) {
			for (auto& anim : myScene.animators) {
				anim.tick(diff.asSeconds());
			}
		}

		// Update the scene.
		float dt = diff.asSeconds();
		for (auto& dice : myScene.objects) {
			// only drop the dice if the object isMoving and if user pressed space
			if (dice.isMoving && throwDice) {
				dice.tick(dt);

				// Check if the dice has hit the table (I need to figure out collisions, hard coded for now)
				if (dice.getPosition().y <= .55f) {
					glm::vec3 pos = dice.getPosition();
					glm::vec3 vel = dice.getVelocity();
					pos.y = 0.55f;
					// make the velocity lose "energy" guess and check values (found 0.5 for y and 0.7 for z and x simulate dice rolling well enough)
					vel.y *= -dice.getBounceCoeff();
					vel.x *= 0.7f;
					vel.z *= 0.7f;

					// lose rotational speed
					dice.setAngularVelocity(dice.getAngularVelocity() * 0.9f);

					dice.setPosition(pos);
					dice.setVelocity(vel);
					if (glm::length(vel) < 0.1f) {
						dice.setVelocity(glm::vec3(0.0f));
						auto rotVel = dice.getAngularVelocity();
						dice.setAngularVelocity(rotVel * 0.9f);
						if (glm::length(rotVel) < 0.1f) {
							snapToNearestRotation(dice);
							dice.isMoving = false; // set to false so my program runs faster i think?
						}
					}
				}
			}
		}

		// Clear the OpenGL "context".
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Render the scene objects.
		for (auto& o : myScene.objects) {
			o.render(myScene.program);
		}
		window.display();
	}

	return 0;
}


