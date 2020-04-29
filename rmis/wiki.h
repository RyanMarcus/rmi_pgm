#include <cstddef>
#include <cstdint>
namespace wiki {
bool load(char const* dataPath);
void cleanup();
const size_t RMI_SIZE = 50331680;
const uint64_t BUILD_TIME_NS = 17085845461;
const char NAME[] = "wiki";
uint64_t lookup(uint64_t key, size_t* err);
}
