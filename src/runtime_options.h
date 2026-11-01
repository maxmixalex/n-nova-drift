#pragma once
#include <stdint.h>
namespace nova {
// Session options copied from the independently versioned user preferences.
// Bits: drone / mine / turret / ally. Manual ability deploys assembled types
// with their automatic bit disabled, without deleting existing constructs.
struct RuntimeOptions { uint8_t auto_deploy_mask,assist_mode; };
}
