#include "LiDAR/LidarShaders.h"

IMPLEMENT_GLOBAL_SHADER(FLidarRayGenShader, "/Plugin/UESensorShaders/Shaders/LidarShaders.usf", "LidarRayGen", SF_RayGen);
IMPLEMENT_GLOBAL_SHADER(FLidarMissShader, "/Plugin/UESensorShaders/Shaders/LidarShaders.usf", "LidarMiss", SF_RayMiss);
IMPLEMENT_GLOBAL_SHADER(FLidarClosestHitShader, "/Plugin/UESensorShaders/Shaders/LidarShaders.usf", "LidarClosestHit", SF_RayHitGroup);
