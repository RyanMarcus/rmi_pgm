#include <cstddef>
#include <cstdint>
namespace fb {
bool load(char const* dataPath);
void cleanup();
const size_t RMI_SIZE = 50331664;
const uint64_t BUILD_TIME_NS = 17919537415;
const char NAME[] = "fb";
uint64_t lookup(uint64_t key, size_t* err);
}
