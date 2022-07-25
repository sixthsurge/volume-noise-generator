#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED

#include <ctime>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using ConfigError = std::runtime_error;

class ChannelConfig {
public:
	enum class Mode {
		Perlin,
		Worley,
		PerlinWorley,
		BlueNoise,
		Curl
	};

	void loadFromFile(const std::vector<std::string>& lines, int& i, int lineCount) {
		auto map = parseStringMap(lines, i, lineCount, '~');

		// Get mode
		if (map.count("mode") == 0) throw ConfigError("Missing option: mode");

		std::map<std::string, Mode> modeMap;
		modeMap["perlin"]       = Mode::Perlin;
		modeMap["worley"]       = Mode::Worley;
		modeMap["perlinWorley"] = Mode::PerlinWorley;
		modeMap["blueNoise"]    = Mode::BlueNoise;
		modeMap["curl"]         = Mode::Curl;

		mode = modeMap[map["mode"]];

		// Post-processing options
		inverted   = getBool (map, "inverted",   false);
		powerCurve = getFloat(map, "powerCurve", 1.0);

		// Channel specific options
		switch (mode) {
		case Mode::PerlinWorley:
			worleyWeight      = getFloat(map, "worleyWeight",      0.3);
			perlinOctaveCount = getInt  (map, "perlinOctaveCount", 1);
			perlinFrequency   = getFloat(map, "perlinFrequency",   10.0f);
			perlinLacunarity  = getFloat(map, "perlinLacunarity",  2.0f);
			perlinPersistence = getFloat(map, "perlinPersistence", 0.5f);
			worleyOctaveCount = getInt  (map, "worleyOctaveCount", 1);
			worleyFrequency   = getFloat(map, "worleyFrequency",   10.0f);
			worleyLacunarity  = getFloat(map, "worleyLacunarity",  2.0f);
			worleyPersistence = getFloat(map, "worleyPersistence", 0.5f);
			break;

		case Mode::BlueNoise:
			blueNoiseRes = getInt(map, "blueNoiseRes", 32);
			zoom = getInt(map, "zoom", 1);
			break;

		case Mode::Curl:
			frequency = getFloat(map, "frequency", 10.0f);
			break;

		default:
			octaveCount = getInt  (map, "octaveCount", 1);
			frequency   = getFloat(map, "frequency", 10.0f);
			lacunarity  = getFloat(map, "lacunarity", 2.0f);
			persistence = getFloat(map, "persistence", 0.5f);
			break;
		}
	}

	ChannelConfig& reseed() {
		static uint randomState = time(NULL);
		seed = randomState = lowbias32(randomState);
		return *this;
	}

	Mode mode;
	uint seed;
	int octaveCount;
	float frequency;
	float lacunarity;
	float persistence;

	// Post-processing options
	bool inverted;
	float powerCurve;

	// Perlin-worley noise options
	float worleyWeight;
	int worleyOctaveCount;
	float worleyFrequency;
	float worleyLacunarity;
	float worleyPersistence;
	int perlinOctaveCount;
	float perlinFrequency;
	float perlinLacunarity;
	float perlinPersistence;

	// Blue noise options
	uint blueNoiseRes;
	int zoom;

private:
	using StringMap = std::map<std::string, std::string>;

	StringMap parseStringMap(const std::vector<std::string>& lines, int& i, int lineCount, char sectionDelim) {
		std::map<std::string, std::string> map;

		for (; i < lineCount; ++i) {
			auto line = lines[i];

			if (line[0] == sectionDelim) break;
			if (line[0] == '#') continue; // Allow for comments

			auto colonPos = line.find_first_of(':');

			if (colonPos == std::string::npos) continue;

			auto key = line.substr(0, colonPos);
			auto val = line.substr(colonPos + 1, line.length() - colonPos);

			map[key] = val;
		}

		--i; // Allow Config to discover ~

		return map;
	}

	int getInt(const StringMap& map, std::string name) {
		if (map.count(name)) return std::stoi(map.at(name));
		else throw ConfigError("Missing option: " + name);
	}

	int getInt(const StringMap& map, std::string name, int defaultValue) {
		if (map.count(name)) return std::stoi(map.at(name));
		else return defaultValue;
	}

	float getFloat(const StringMap& map, std::string name) {
		if (map.count(name)) return std::stof(map.at(name));
		else throw ConfigError("Missing option " + name);
	}

	float getFloat(const StringMap& map, std::string name, float defaultValue) {
		if (map.count(name)) return std::stof(map.at(name));
		else return defaultValue;
	}

	bool getBool(const StringMap& map, std::string name) {
		if (map.count(name)) return map.at(name).find("true") != std::string::npos;
		else throw ConfigError("Missing option " + name);
	}

	bool getBool(const StringMap& map, std::string name, bool defaultValue) {
		if (map.count(name)) return map.at(name).find("true") != std::string::npos;
		else return defaultValue;
	}
};

class Config {
public:
	Config(std::string name) :
		name(name)
	{
		try {
			std::ifstream file("input/" + name + ".txt");

			std::vector<std::string> lines;
			for (std::string line; std::getline(file, line);) lines.push_back(line);
			file.close();

			if (lines.empty()) throw ConfigError("File is empty or does not exist");

			// First line contains texture size in format {width}x{height}x{depth}
			auto delim0 = lines[0].find_first_of('x');
			auto delim1 = lines[0].find_last_of('x');

			if (delim0 == std::string::npos || delim1 == std::string::npos) {
				throw ConfigError("Invalid texture size description");
			}

			width  = stoi(lines[0].substr(0, delim0));
			height = stoi(lines[0].substr(delim0 + 1, delim1 - delim0 - 1));
			depth  = stoi(lines[0].substr(delim1 + 1, lines[0].length() - delim1));

			int lineCount = lines.size();
			for (int i = 0; i < lineCount; ++i) {
				// ~ means next channel
				if (lines[i][0] == '~') {
					int n = 1;

					if (lines[i].find_first_of("2") != std::string::npos) n = 2;
					if (lines[i].find_first_of("3") != std::string::npos) n = 3;
					if (lines[i].find_first_of("4") != std::string::npos) n = 4;

					ChannelConfig c;
					c.loadFromFile(lines, ++i, lineCount);

					for (int j = 0; j < n; ++j) m_channelConfigs.push_back(c.reseed());
				}
			}

			if (getChannelCount() > 4) throw ConfigError("Invalid number of channels");

		} catch(const std::exception& e) {
			std::cerr << "Failed to load configuration " << name << ":\n";
			std::cerr << e.what() << std::endl;
		}
	}

	ChannelConfig& getChannelConfig(int index) {
		return m_channelConfigs[index];
	}

	int getChannelCount() const {
		return m_channelConfigs.size();
	}

	const std::string name;
	int width;
	int height;
	int depth;

private:
	std::vector<ChannelConfig> m_channelConfigs;
};

#endif // CONFIG_INCLUDED
