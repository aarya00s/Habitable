#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float life;
};

class ParticleSystem {
public:
    ParticleSystem(unsigned int numParticles) : particles(numParticles) {}

    void update(float deltaTime) {
        for (Particle& particle : particles) {
            // Update the particle's position based on its velocity
            particle.position += particle.velocity * deltaTime;

            // Decrease the particle's life
            particle.life -= deltaTime;

            // If the particle's life has run out, reset it
            if (particle.life <= 0.0f) {
                resetParticle(particle);
            }
        }
    }

    void draw() {
        for (const Particle& particle : particles) {
            // Draw the particle here
            // You'll need to replace this with actual OpenGL code
            drawParticle(particle);
        }
    }

private:
    std::vector<Particle> particles;

#include <random>

    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

    glm::vec3 generateRandomPosition() {
        // Generate a random position within a certain radius
        float radius = 10.0f; // Adjust this value to change the size of the nebula
        glm::vec3 position = glm::vec3(distribution(generator), distribution(generator), distribution(generator));
        position = glm::normalize(position) * radius * sqrt(distribution(generator));
        return position;
    }

    glm::vec3 generateRandomVelocity() {
        // Generate a small random velocity
        float speed = 0.1f; // Adjust this value to change the speed of the nebula's evolution
        glm::vec3 velocity = glm::vec3(distribution(generator), distribution(generator), distribution(generator));
        velocity = glm::normalize(velocity) * speed;
        return velocity;
    }

    glm::vec4 generateRandomColor() {
        // Generate a color that's mostly pink, but with a bit of variation
        float r = 0.8f + 0.2f * distribution(generator); // Pink to red
        float g = 0.3f + 0.2f * distribution(generator); // Dark pink to light pink
        float b = 0.5f + 0.5f * distribution(generator); // Dark pink to white
        float a = 1.0f; // Fully opaque
        return glm::vec4(r, g, b, a);
    }

    float generateRandomLife() {
        // Generate a random life duration
        float minLife = 5.0f; // Adjust this value to change the minimum life duration
        float maxLife = 10.0f; // Adjust this value to change the maximum life duration
        float life = minLife + distribution(generator) * (maxLife - minLife);
        return life;
    }
};
