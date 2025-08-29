#include "LiDAR/LidarShaders.h"

// Implement LiDAR global shaders
IMPLEMENT_GLOBAL_SHADER(FLidarRayGenShader, "/Shaders/LiDAR/LidarShaders.usf", "LidarRayGen", SF_RayGen);
IMPLEMENT_GLOBAL_SHADER(FLidarMissShader, "/Shaders/LiDAR/LidarShaders.usf", "LidarMiss", SF_RayMiss);
IMPLEMENT_GLOBAL_SHADER(FLidarClosestHitShader, "/Shaders/LiDAR/LidarShaders.usf", "LidarClosestHit", SF_RayHitGroup);
