#include <cstdint>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

using namespace glm;

#include "utility.hpp"
#include "blueNoise.hpp"
#include "config.hpp"
#include "texture.hpp"

//------------------------------------------------------------------------------------------------//

float getWorleyNoise(uint seed, vec3 pos, vec3 repeat) {
	auto tile = glm::floor(pos);

	float distance = 1.0;

	for (int z = -1; z <= 1; ++z) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				auto neighbour = tile + glm::vec3(x, y, z);

				vec3 repeatedNeighbour;
				repeatedNeighbour.x = wrap(neighbour.x, 0.0f, repeat.x);
				repeatedNeighbour.y = wrap(neighbour.y, 0.0f, repeat.y);
				repeatedNeighbour.z = wrap(neighbour.z, 0.0f, repeat.y);

				auto neighbourIndex = (uint32_t) ((repeatedNeighbour.z + repeat.y * repeatedNeighbour.y) * repeat.x + repeatedNeighbour.x);

				auto xHash = lowbias32(seed + neighbourIndex);
				auto yHash = lowbias32(xHash);
				auto zHash = lowbias32(yHash);

				glm::vec3 featurePoint = neighbour + glm::vec3(xHash, yHash, zHash) / (float) uint32Max;

				distance = min(distance, glm::distance(pos, featurePoint));
			}
		}
	}

	return distance;
}

float getPerlinNoise(uint seed, vec3 pos, vec3 repeat) {
	vec4 pos4D = vec4(pos.x, pos.y, pos.z, 0.0);
	vec4 repeat4D = vec4(repeat.x, repeat.y, repeat.z, glm::max(glm::max(repeat.x, repeat.y), repeat.z));

	// Offset position by seed along w axis since GLM does not allow us to set a seed for the noise
	// % 1000 is there to prevent FP precision errors
	pos4D.w += (float) 1.618033 * (seed % 1000);

	return perlin(pos4D, repeat4D) * 0.5 + 0.5;
}

vec3 getPerlinNoiseTriplet(uint seed, vec3 pos, vec3 repeat) {
	vec3 noise;
	noise.x = getPerlinNoise(seed, pos, repeat),
	noise.y = getPerlinNoise(seed = lowbias32(seed), pos, repeat);
	noise.z = getPerlinNoise(seed = lowbias32(seed), pos, repeat);
	return noise;
}

vec3 getCurlNoise(uint seed, vec3 pos, vec3 repeat) {
	// https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph2007-curlnoise.pdf
	// https://www.bit-101.com/blog/2021/07/curl-noise-demystified/ (explanation for 2D case)
	const float h = 1e-3;

	vec3 dFdx = getPerlinNoiseTriplet(seed, pos + glm::vec3(h, 0.0f, 0.0f), repeat)
	          - getPerlinNoiseTriplet(seed, pos - glm::vec3(h, 0.0f, 0.0f), repeat);
	     dFdx /= 2.0f * h;

	vec3 dFdy = getPerlinNoiseTriplet(seed, pos + glm::vec3(0.0f, h, 0.0f), repeat)
	          - getPerlinNoiseTriplet(seed, pos - glm::vec3(0.0f, h, 0.0f), repeat);
	     dFdy /= 2.0f * h;

	vec3 dFdz = getPerlinNoiseTriplet(seed, pos + glm::vec3(0.0f, 0.0f, h), repeat)
	          - getPerlinNoiseTriplet(seed, pos - glm::vec3(0.0f, 0.0f, h), repeat);
	     dFdz /= 2.0f * h;

	vec3 velocity;
	velocity.x = dFdy.z - dFdz.y;
	velocity.y = dFdz.x - dFdx.z;
	velocity.z = dFdx.y - dFdy.x;

	velocity /= sqrt(2.0);

	return velocity * 0.5f + 0.5f;
}

float fBm(
	std::function<float(uint32_t, glm::vec3, glm::vec3)> noiseFunction,
	uint seed,
	glm::vec3 pos,
	glm::vec3 repeat,
	int octaveCount,
	float frequency,
	float lacunarity,
	float persistence
) {
	if (octaveCount <= 0) return 0.0;

	float amplitude = 1.0;
	float noiseSum = 0.0;
	float amplitudeSum = 0.0;

	for (int octaveIndex = 0; octaveIndex < octaveCount; ++octaveIndex) {
		float noise = noiseFunction(seed, pos * frequency, repeat * frequency);

		noiseSum += amplitude * noise;
		amplitudeSum += amplitude;
		amplitude *= persistence;
		frequency *= lacunarity;
		seed = lowbias32(seed);
	}

	noiseSum /= amplitudeSum;

	return glm::clamp(noiseSum, 0.0f, 1.0f);
}

/*
// Perlin-worley noise according to https://github.com/greje656/clouds
// This is different to how Schneidner presented it
float blendPerlinWorley(float perlinNoise, float worleyNoise, float worleyWeight) {
	float curve = 0.75;
	if(worleyWeight < 0.5) {
		worleyWeight = worleyWeight * 2.0;
		float noise = perlinNoise + worleyNoise * worleyWeight;
		return noise * glm::mix(1.0, 0.5, glm::pow(worleyWeight, curve));
	} else {
		worleyWeight = (worleyWeight-0.5)/0.5;
		float noise = worleyNoise + perlinNoise * (1.0 - worleyWeight);
		return noise * glm::mix(0.5, 1.0, glm::pow(worleyWeight, 1.0 / curve));
	}
}
*/

float getNoise(ChannelConfig& channelConfig, vec3 pos, int channel) {
	float noise;

	switch (channelConfig.mode) {
	case ChannelConfig::Mode::Perlin:
		noise = fBm(
			getPerlinNoise,
			channelConfig.seed,
			pos,
			glm::vec3(1.0f),
			channelConfig.octaveCount,
			channelConfig.frequency,
			channelConfig.lacunarity,
			channelConfig.persistence
		);
		break;

	case ChannelConfig::Mode::Worley: {
		noise = fBm(
			getWorleyNoise,
			channelConfig.seed,
			pos,
			glm::vec3(1.0f),
			channelConfig.octaveCount,
			channelConfig.frequency,
			channelConfig.lacunarity,
			channelConfig.persistence
		);
		break;
	}

	case ChannelConfig::Mode::PerlinWorley: {
		float perlin = fBm(
			getPerlinNoise,
			channelConfig.seed,
			pos,
			glm::vec3(1.0f),
			channelConfig.perlinOctaveCount,
			channelConfig.perlinFrequency,
			channelConfig.perlinLacunarity,
			channelConfig.perlinPersistence
		);

		float worley = 1.0 - fBm(
			getWorleyNoise,
			channelConfig.seed,
			pos,
			glm::vec3(1.0f),
			channelConfig.worleyOctaveCount,
			channelConfig.worleyFrequency,
			channelConfig.worleyLacunarity,
			channelConfig.worleyPersistence
		);

		noise = linearStep((1.0f - worley) * channelConfig.worleyWeight, 1.0f, perlin);
		break;
	}

	case ChannelConfig::Mode::BlueNoise:
		noise = getBlueNoise(pos / float(channelConfig.zoom), channel, channelConfig.blueNoiseRes);
		break;

	case ChannelConfig::Mode::Curl:
		noise = getCurlNoise(
			channelConfig.seed,
			pos * channelConfig.frequency,
			vec3(channelConfig.frequency)
		)[channel];
		break;

	default:
		noise = 0.5f;
	}

	noise = channelConfig.inverted ? 1.0 - noise : noise;
	noise = glm::pow(noise, channelConfig.powerCurve);

	return noise;
}

void generateNoiseTexture(const std::string& name) {
	Config config(name);

	uvec3 size(config.width, config.height, config.depth);
	int channelCount = config.getChannelCount();

	VolumeTexture texture(size, channelCount);

	texture.process(
		[&] (vec3 pos, uint channel) {
			auto& channelConfig = config.getChannelConfig(channel);

			return getNoise(channelConfig, pos, channel);
		}
	);

	texture.writeFile("output/" + name + ".dat");
	texture.writeSlice("output/" + name + "Slice.png", 0);
}

int main(int argc, char** argv) {
	for (int argument = 1; argument < argc; ++argument) {
		std::string name(argv[argument]);
		generateNoiseTexture(name);
	}
}
