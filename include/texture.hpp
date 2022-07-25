#ifndef TEXTURE_INCLUDED
#define TEXTURE_INCLUDED

#include <fstream>
#include <functional>
#include <memory>

class VolumeTexture {
public:
	VolumeTexture(uvec3 size, int channels) :
		m_size(size),
		m_channels(channels),
		m_data(std::make_unique<std::uint8_t[]>(size.x * size.y * size.z * channels)) {}

	// Load volume texture from a series of files representing xy slices of the texture
	// loadFromSlices("slice#.png") will load slice0.png, slice1.png, ..., sliceN.png
	bool loadFromSlices(std::string generalPath) {
		auto hashIndex = generalPath.find('#');

		for (int i = 0; i < m_size.z; ++i) {
			int width, height, channels;

			std::string path = generalPath;
			path.replace(hashIndex, 1, std::to_string(i));

			unsigned char* sliceData = stbi_load(path.c_str(), &width, &height, &channels, m_channels);

			if (sliceData == NULL || width != m_size.x || height != m_size.y)
				throw std::runtime_error("Failed to load texture slice " + path);

			auto n = m_size.x * m_size.y * m_channels;
			auto destination = m_data.get() + i * n;

			memcpy(destination, sliceData, n);

			stbi_image_free(sliceData);
		}

		return true;
	}

	std::uint8_t& operator[](uvec4 pos) {
		// pos = min(pos, uvec4(m_size.x, m_size.y, m_size.z, m_channels));

		auto index = getIndex(xyz(pos), pos.w);
		return m_data[index];
	}

	uint8_t operator[](uvec4 pos) const {
		auto index = getIndex(xyz(pos), pos.w);
		return m_data[index];
	}

	void process(std::function<float(vec3, uint)> f) {
		int index = 0;

		for (uint z = 0; z < m_size.z; ++z) {
			for (uint y = 0; y < m_size.y; ++y) {
				for (uint x = 0; x < m_size.x; ++x) {
					vec3 pos = vec3(x, y, z) / vec3(m_size);

					for (uint channel = 0; channel < m_channels; ++channel) {
						float v = f(pos, channel);

						m_data[index++] = unormToByte(v);
					}
				}
			}
		}
	}

	bool writeFile(const std::string& path) const {
		try {
			std::ofstream file(path, std::ios_base::binary | std::ios_base::out);
			file.write(reinterpret_cast<const char*>(m_data.get()), m_channels * m_size.x * m_size.y * m_size.z);
			file.close();
		} catch(const std::exception& e) {
			std::cout << e.what();
			return false;
		}
		return true;
	}

	bool writeSlice(const std::string& path, int slice) const {
		auto n = m_size.x * m_size.y * m_channels;
		auto ptr = m_data.get() + slice * n;

		return stbi_write_png(
			path.c_str(),
			m_size.x,
			m_size.y,
			m_channels,
			ptr,
			m_size.x * m_channels
		) != 0;
	}

private:
	uint getIndex(uvec3 pos, uint channel) const {
		auto pixel = (pos.z * m_size.y + pos.y) * m_size.x + pos.x;
		return pixel * m_channels + channel;
	}

	static uint8_t unormToByte(float v) {
		v = clamp(v, 0.0f, 1.0f);
		v = floor(v * 255.99f);
		return static_cast<uint8_t>(v);
	}

	std::unique_ptr<std::uint8_t[]> m_data;

	const uvec3 m_size;
	const uint m_channels;
};

#endif // TEXTURE_INCLUDED
