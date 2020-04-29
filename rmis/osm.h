#include <cstddef>
#include <cstdint>
namespace osm {
bool load(char const* dataPath);
void cleanup();
const size_t RMI_SIZE = 50331680;
const uint64_t BUILD_TIME_NS = 17796564950;
const char NAME[] = "osm";
uint64_t lookup(uint64_t key, size_t* err);
}
