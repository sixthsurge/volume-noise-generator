# Volume Noise Generator

## Instructions

1. Install GCC/MinGW and execute `build.sh`/`build.bat`
2. Create a config file `name.txt` in the input folder. See the existing `example.txt` for an example
3. Run `./generate name` where name is the name of your config file without the ".txt"

The resulting texture is provided as a binary data file which can be loaded into OptiFine as follows:
```
texture.<prepare/composite/deferred>.<sampler of choice> = path/to/texture.dat TEXTURE_3D <R8/RG8/RGB8/RGBA8> <sizeX> <sizeY> <sizeZ> <RED/RG/RGB/RGBA> UNSIGNED_BYTE
```

## Config file format

(See example config file)

~ means next channel

option:value

mode | Determines the type of noise to generate in this channel. Possible values are "perlin", "worley", "perlinWorley", "blueNoise" and "curl"

### General options (perlin or worley noise)
* seed
* inverted
* octaveCount
* frequency
* lacunarity
* persistence

### Perlin-worley noise options
* worleyWeight
* worleyOctaveCount
* worleyFrequency
* worleyLacunarity
* worleyPersistence
* perlinOctaveCount
* perlinFrequency
* perlinLacunarity
* perlinPersistence

## Blue noise options
* blueNoiseRes | Resolution of the blue noise texture to use. Must be 16, 32 or 64, and must match the texture resolution
* zoom | This is supposed to scale up the blue noise texture. Currently it just causes a segmentation fault

**Note: lacunarity must be a power of 2 for the texture to tile properly**
