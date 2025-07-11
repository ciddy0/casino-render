#pragma once
#include <memory>
#include "ShaderProgram.h"
#include "Mesh3D.h"
class Object3D {
private:
	// The object's list of meshes and children.
	std::vector<Mesh3D> m_meshes;
	std::vector<Object3D> m_children;

	// The object's position, orientation, and scale in world space.
	glm::vec3 m_position;
	glm::vec3 m_orientation;
	glm::vec3 m_scale;
	glm::vec3 m_center;

	// physics
	glm::vec3 m_velocity{0.0f};
	glm::vec3 m_angularVelocity{0.0f};
	glm::vec3 m_acceleration{0.0f, 0.0, 0.0f}; // Default gravity
	float m_bounceCoeff = 0.0f;

	// The object's material.
	glm::vec4 m_material;

	// The object's base transformation matrix.
	glm::mat4 m_baseTransform;

	// Some objects from Assimp imports have a "name" field, useful for debugging.
	std::string m_name;

	// Recomputes the local->world transformation matrix.
	glm::mat4 buildModelMatrix() const;


public:
	bool isMoving;
	// No default constructor; you must have a mesh to initialize an object.
	Object3D() = delete;

	Object3D(std::vector<Mesh3D>&& meshes);
	Object3D(std::vector<Mesh3D>&& meshes, const glm::mat4& baseTransform);

	// Simple accessors.
	const glm::vec3& getPosition() const;
	const glm::vec3& getOrientation() const;
	const glm::vec3& getScale() const;
	const glm::vec3& getCenter() const;
	const std::string& getName() const;
	const glm::vec4& getMaterial() const;
	const glm::vec3& getVelocity() const;
	const glm::vec3& getAngularVelocity() const;
	const float getBounceCoeff() const;


	// Child management.
	size_t numberOfChildren() const;
	const Object3D& getChild(size_t index) const;
	Object3D& getChild(size_t index);


	// Simple mutators.
	void setPosition(const glm::vec3& position);
	void setOrientation(const glm::vec3& orientation);
	void setScale(const glm::vec3& scale);
	void setCenter(const glm::vec3& center);
	void setName(const std::string& name);
	void setMaterial(const glm::vec4& material);
	void setAcceleration(const glm::vec3& acceleration);
	void setVelocity(const glm::vec3& velocity);
	void setAngularVelocity(const glm::vec3& angularVelocity);
	void setBounceCoeff(const float bounceCoeff);

	// Transformations.
	void move(const glm::vec3& offset);
	void rotate(const glm::vec3& rotation);
	void grow(const glm::vec3& growth);
	void addChild(Object3D&& child);

	// Rendering.
	void render(ShaderProgram& shaderProgram) const;
	void renderRecursive(ShaderProgram& shaderProgram, const glm::mat4& parentMatrix) const;

	// physics
	void tick(float dt);
};