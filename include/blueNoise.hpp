#ifndef BLUENOISE_INCLUDED
#define BLUENOISE_INCLUDED

#include <map>
#include <vector>

#include "texture.hpp"

class BlueNoiseTextures {
public:
	static const VolumeTexture& get(int res) {
		return getInstance().getTexture(res);
	}

private:
	static BlueNoiseTextures& getInstance() {
		static BlueNoiseTextures instance;
		return instance;
	}

	static std::string getPath(uint res) {
		std::string s = std::to_string(res);
		return "input/blueNoiseTextures/" + s + "_" + s + "_" + s + "/LDR_RGBA_#.png";
	}

	const VolumeTexture& getTexture(int res) {
		if (m_textureMapping.count(res)) {
			return *m_textureMapping[res];
		} else {
			auto& texture = m_textureStorage.emplace_back(uvec3(res), 4);
			texture.loadFromSlices(getPath(res));
			m_textureMapping[res] = &texture;
			return texture;
		}
	}

	std::vector<VolumeTexture> m_textureStorage;
	std::map<int, const VolumeTexture*> m_textureMapping;
};

float getBlueNoise(vec3 pos, int channel, int res) {
	pos  *= float(res);

	uvec4 index;
	index.x = uint(pos.x);
	index.y = uint(pos.y);
	index.z = uint(pos.z);
	index.w = channel;

	return BlueNoiseTextures::get(res)[index] / 256.0;
}

#endif // BLUENOISE_INCLUDED
