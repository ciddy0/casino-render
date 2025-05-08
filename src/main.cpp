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
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>

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

Scene Casino() {
	Scene scene{ phongLightingShader()
	};
	std::vector<Texture> floorTextures = {
		loadTexture("models/carpet.jpeg", "baseTexture"),
	};
	// the floor of my scene
	auto floorMesh = Mesh3D::square(floorTextures);
	auto floor = Object3D(std::vector<Mesh3D>{floorMesh});
	floor.grow(glm::vec3(10, 10, 10));
	floor.move(glm::vec3(0, 0, 0));
	floor.rotate(glm::vec3(-M_PI / 2, 0, 0));
	scene.objects.push_back(std::move(floor));

	// pool table
	auto poolTable = assimpLoad("models/pool_table/scene.gltf", true);
	poolTable.grow(glm::vec3(0.002));
	poolTable.rotate(glm::vec3(0, -M_PI/2, 0));
	poolTable.move(glm::vec3(-2, .3, -3));
	scene.objects.push_back(std::move(poolTable));

	// the table where the dice fall onto
	auto table = assimpLoad("models/poker_table/scene.gltf", true);
	table.setScale(glm::vec3(.001));
	table.setPosition(glm::vec3(0, 0, 0));
	scene.objects.push_back(std::move(table));

	// casino chips
	auto casinoChips = assimpLoad("models/casino_chips/scene.gltf", true);
	casinoChips.setScale(glm::vec3(1));
	casinoChips.setPosition(glm::vec3(.4, .6, 0));
	scene.objects.push_back(std::move(casinoChips));

	// slot machine (i wish i found a better looking one :c)
	auto slots2 = assimpLoad("models/slotmachine3/scene.gltf", true);
	slots2.setScale(glm::vec3(2));
	slots2.setPosition(glm::vec3(0, 0.8, -4));
	slots2.rotate(glm::vec3(0, -M_PI/2, 0));
	scene.objects.push_back(std::move(slots2));

	// animation for my hierarchical slot machine
	Animator animLever;
	Animator animLeverUp;
	Animator animWheel1;
	Animator animWheel2;
	Animator animWheel3;
	animLever.addAnimation(std::make_unique<PauseAnimation>(scene.objects[4], 1.5));
	animLever.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(1).getChild(0), 1, glm::vec3(0, 0, .5*(-2*M_PI))));
	animLever.addAnimation(std::make_unique<PauseAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(1).getChild(0), .5));
	animLever.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(1).getChild(0), 1, glm::vec3(0, 0, .5*(2*M_PI))));

	animWheel1.addAnimation(std::make_unique<PauseAnimation>(scene.objects[4], 1.5));
	animWheel1.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(2).getChild(0), 3, glm::vec3(0, 10*(-2*M_PI), 0)));
	animWheel2.addAnimation(std::make_unique<PauseAnimation>(scene.objects[4], 1.5));
	animWheel2.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(3).getChild(0),5 , glm::vec3(0, 2*(-2*M_PI), 0)));
	animWheel3.addAnimation(std::make_unique<PauseAnimation>(scene.objects[4], 1.5));
	animWheel3.addAnimation(std::make_unique<RotationAnimation>(scene.objects[4].getChild(0).getChild(0).getChild(4).getChild(0), 7, glm::vec3(0, -2*M_PI, 0)));

	// die #1
	auto cube = assimpLoad("models/dice/scene.gltf", true);
	cube.setScale(glm::vec3(.05));
	cube.move(glm::vec3(0, 2, 0));
	cube.setAcceleration(glm::vec3(0, -9.8, 0));
	cube.setVelocity(glm::vec3(-.5, .5, 0));
	cube.setAngularVelocity(glm::vec3(8, 5, 2));
	cube.setBounceCoeff(0.5);
	cube.isMoving = true;
	scene.objects.push_back(std::move(cube));

	// die #2
	auto cube2 = assimpLoad("models/dice/scene.gltf", true);
	cube2.setScale(glm::vec3(.05));
	cube2.move(glm::vec3(-.5, 2, 0));
	cube2.setAcceleration(glm::vec3(0, -9.8, 0));
	cube2.setVelocity(glm::vec3(.5, .5, -.5));
	cube2.setAngularVelocity(glm::vec3(12, 1, 5));
	cube2.setBounceCoeff(0.5);
	cube2.isMoving = true;
	scene.objects.push_back(std::move(cube2));

	// letter g
	auto letterG = assimpLoad("models/g_letter/scene.gltf", true);
	letterG.setScale(glm::vec3(.5));
	letterG.move(glm::vec3(-.5, 2, 3));
	scene.objects.push_back(std::move(letterG));

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

	// deck of cards
	auto cardDeck = assimpLoad("models/deck_of_cards/scene.gltf", true);
	cardDeck.grow(glm::vec3(0.001));
	cardDeck.move(glm::vec3(.4, .6, 0));
	scene.objects.push_back(std::move(cardDeck));

	// roulette table
	auto rouletteTable = assimpLoad("models/roulette_table/scene.gltf", true);
	rouletteTable.grow(glm::vec3(.3));
	rouletteTable.move(glm::vec3(3, .8, -2.5));
	rouletteTable.rotate(glm::vec3(0, -M_PI/2, 0));
	scene.objects.push_back(std::move(rouletteTable));

	// different poker table
	auto pokerTable2 = assimpLoad("models/poker_table2/scene.gltf", true);
	pokerTable2.grow(glm::vec3(1));
	pokerTable2.move(glm::vec3(3, -1.5, 0));
	pokerTable2.isMoving = false;
	scene.objects.push_back(std::move(pokerTable2));

	// bar
	auto bar = assimpLoad("models/art_deco_bar/scene.gltf", true);
	bar.grow(glm::vec3(.8));
	bar.move(glm::vec3(3, 0, -4.6));
	bar.isMoving = false;
	scene.objects.push_back(std::move(bar));

	// textures for my walls and ceiling
	std::vector<Texture> WallTextures = {
		loadTexture("models/casino_left.jpg", "baseTexture"),
	};
	std::vector<Texture> WallTextures2 = {
		loadTexture("models/whitewall.jpg", "baseTexture"),
	};
	std::vector<Texture> ceilingTextures = {
		loadTexture("models/popcorn_ceiling.jpg", "baseTexture"),
	};

	// left wall
	auto leftWallMesh = Mesh3D::square(WallTextures);
	auto leftWall = Object3D(std::vector<Mesh3D>{leftWallMesh});
	leftWall.grow(glm::vec3(10, 10, 10));
	leftWall.move(glm::vec3(-5, 4.5, 0));
	leftWall.rotate(glm::vec3(0, M_PI/2, 0));
	scene.objects.push_back(std::move(leftWall));

	// right wall
	auto rightWallMesh = Mesh3D::square(WallTextures);
	auto rightWall = Object3D(std::vector<Mesh3D>{rightWallMesh});
	rightWall.grow(glm::vec3(10, 10, 10));
	rightWall.move(glm::vec3(5, 4.5, 0));
	rightWall.rotate(glm::vec3(0, -M_PI/2, 0));
	scene.objects.push_back(std::move(rightWall));

	// front wall
	auto frontWallMesh = Mesh3D::square(WallTextures2);
	auto frontWall = Object3D(std::vector<Mesh3D>{frontWallMesh});
	frontWall.grow(glm::vec3(10, 10.8, 10));
	frontWall.move(glm::vec3(0, 4.4, -5));
	frontWall.rotate(glm::vec3(0, 0, 0));
	scene.objects.push_back(std::move(frontWall));

	// back wall
	auto backWallMesh = Mesh3D::square(WallTextures2);
	auto backWall = Object3D(std::vector<Mesh3D>{backWallMesh});
	backWall.grow(glm::vec3(10, 10.8, 10));
	backWall.move(glm::vec3(0, 4.4, 5));
	backWall.rotate(glm::vec3(0, M_PI, 0));
	scene.objects.push_back(std::move(backWall));

	// ceiling
	auto ceilingMesh = Mesh3D::square(ceilingTextures);
	auto ceiling = Object3D(std::vector<Mesh3D>{ceilingMesh});
	ceiling.grow(glm::vec3(10, 10, 10));
	ceiling.move(glm::vec3(0, 5, 0));
	ceiling.rotate(glm::vec3(-M_PI / 2, 0, M_PI));
	scene.objects.push_back(std::move(ceiling));

	// animation for my letters
	glm::vec3 p0 = glm::vec3(-.5, 2, 3);
	glm::vec3 p1 = glm::vec3(0, .5, 0);
	glm::vec3 p2 = glm::vec3(1, 0, 0);
	glm::vec3 p3_g = glm::vec3(-.5, 2, 0);
	glm::vec3 p3_a = glm::vec3(-.2, 2, 0);
	glm::vec3 p3_t = glm::vec3(0.1, 2, 0);
	glm::vec3 p3_o = glm::vec3(.4, 2, 0);

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

	// basically makes it so its increments of 90 degrees
	// ex: if our dice is at 45 degrees then 45/90=0.5
	// round(0.5) = 1 -> 1 * 90 = 90 degrees
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
	// learnopenGL only the front faces are rendered with culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);

	// Inintialize scene objects.
	auto myScene = Casino();

	// Activate the shader program.
	myScene.program.activate();

	// load sound files into bufffer using SFML audio library (googled documentation)
	sf::SoundBuffer diceBuffer, soundBuffer, winBuffer;
	if (!diceBuffer.loadFromFile("sounds/dice.flac") || !soundBuffer.loadFromFile("sounds/coin-inserting.wav")
		|| !winBuffer.loadFromFile("sounds/win.wav")) {
		std::cerr << "Failed to load audio :c" << std::endl;
	}
	sf::Sound diceSound, coinSound, winSound;
	diceSound.setBuffer(diceBuffer);
	coinSound.setBuffer(soundBuffer);
	winSound.setBuffer(winBuffer);

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

	// camera view (from lecture) we can create the camera outside the while loop so we dont have to compute everytime unless we move around
	glm::vec3 cameraPos = glm::vec3(0, 1.3, 2);
	glm::vec3 cameraDir = glm::vec3(0, 0, -1);
	glm::mat4 camera = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0, 1, 0));
	glm::mat4 perspective = glm::perspective(glm::radians(45.0), static_cast<double>(window.getSize().x) / window.getSize().y, 0.1, 100.0);
	float cameraSpeed = 0;
	auto animationTimeElapsed = 0.0;
	bool winSoundActive = true;
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
					coinSound.play();
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
		std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;
		last = now;

		// using our fps we can set a smoother camera speed
		cameraSpeed = 100.0f * diff.asSeconds();

		myScene.program.setUniform("view", camera);
		myScene.program.setUniform("projection", perspective);
		myScene.program.setUniform("cameraPos", cameraPos);

		//  ambient, diffuse, specular, shininess
		myScene.program.setUniform("material", glm::vec4(.6,.5,.5,0));
		// ambient color (going for like a yellowish color)
		myScene.program.setUniform("ambientColor", glm::vec3(.8,.8,.5));
		// light direction that points downward
		myScene.program.setUniform("directionalLight", glm::vec3(0,-1,0));
		// color of directional light softer yellow
		myScene.program.setUniform("directionalColor", glm::vec3(.4,.4,.2));

		// when the user click return start the animations
		if (startAnimation) {
			for (auto& anim : myScene.animators) {
				anim.tick(diff.asSeconds());
				// wasn't sure how to access currentTime() so i just used a counter
				animationTimeElapsed += .1;
				// after 7s of when the animation started play the win sound
				if (animationTimeElapsed > 2800 && winSoundActive) {
					winSound.play();
					winSoundActive = false;
				}
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
					diceSound.play();
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
					// square root x,y,x components in velocity to get magnitude
					if (glm::length(vel) < 0.1f) {
						dice.setVelocity(glm::vec3(0.0f));
						auto rotVel = dice.getAngularVelocity();
						dice.setAngularVelocity(rotVel * 0.9f);
						// square root x,y,z in angular velocity to get magnitude
						if (glm::length(rotVel) < 0.1f) {

							// snap the dice to the nearest 90 degrees
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


