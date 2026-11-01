// Original enemySpawnInfo constructors and final follower/pairing assignments.
// Indices use SourceEnemy and SourceDynamicFormation order.
struct DynamicChoices {uint32_t formations;uint8_t entries;uint16_t always_repeat_q8,may_repeat_q8;uint8_t followers[8],pairs[8];};
const DynamicChoices dynamic_choices[SOURCE_ENEMY_COUNT]={
 {0x1u,1,0,0,{255},{255}}, // asteroid
 {0x1u,1,0,0,{255},{255}}, // asteroidMega
 {0x807f2u,1,0,307,{2,255},{3,255}}, // interceptor
 {0x5fbu,1,0,282,{255},{2,255}}, // warden
 {0x1fbu,1,0,0,{255},{255}}, // gyrogun
 {0x3f2u,1,0,0,{255},{4,255}}, // scatterDrone
 {0x803f2u,1,0,0,{6,255},{255}}, // swarmling
 {0x803f2u,1,0,0,{7,255},{255}}, // swarmlingGreen
 {0x100fbu,1,0,0,{10,6,9,3,255},{25,9,6,255}}, // host
 {0x1fbu,1,0,294,{255},{2,3,6,255}}, // beamcaster
 {0x800fbu,1,0,0,{10,6,255},{2,6,9,255}}, // myrmidon
 {0x7bu,3,0,269,{255},{255}}, // pulsar
 {0x41cu,1,896,294,{255},{255}}, // saucer
 {0xfc10u,1,0,282,{255},{255}}, // boltThrower
 {0x8004bu,1,0,0,{14,255},{2,3,255}}, // hammerhead
 {0xfbu,1,0,0,{255},{16,255}}, // snapper
 {0xfbu,1,0,0,{255},{15,255}}, // serpent
 {0xfbu,1,0,0,{255},{4,5,2,3,255}}, // vanguard
 {0x1000bu,4,0,0,{6,10,3,4,2,255},{5,3,4,6,2,255}}, // hive
 {0x1u,1,0,0,{255},{255}}, // crabNest
 {0x7bu,1,0,0,{255},{25,2,3,6,255}}, // tracer
 {0x5f2u,1,0,294,{255},{2,10,3,255}}, // shredderMine
 {0x30019u,1,0,0,{10,21,2,3,255},{2,8,3,10,17,9,255}}, // minelayer
 {0x70003u,5,0,0,{10,11,255},{11,29,8,10,255}}, // eclipse
 {0x70001u,1,0,0,{10,6,4,21,14,255},{4,5,17,10,14,255}}, // constructor
 {0x1u,1,0,0,{255},{255}}, // cherub
 {0xf0001u,1,0,0,{10,6,25,14,20,3,255},{10,25,14,20,3,17,255}}, // bulwark
 {0xf0001u,1,0,0,{10,21,2,25,20,3,255},{10,2,3,25,20,14,255}}, // juggernaut
 {0xb0003u,2,0,0,{6,14,2,3,255},{14,20,255}}, // celestial
 {0x70001u,3,0,0,{10,6,11,255},{11,23,25,255}}, // spitfire
 {0x30001u,1,0,0,{6,11,7,25,255},{11,23,25,7,255}}, // ceratotitan
 {0x1u,1,0,0,{255},{10,25,255}}, // stationOmega
 {0x1u,1,0,0,{10,25,2,3,25,20,14,255},{255}}, // dreadnought
 {0xf0001u,3,0,0,{10,9,2,255},{9,2,10,255}}, // scion
 {0x1u,1,0,0,{255},{255}}, // dweller
 {0xf0001u,1,0,0,{10,4,14,17,255},{4,5,17,14,255}}, // warbringer
 {0x1u,2,0,0,{255},{255}}, // crucible
 {0xf0001u,1,0,0,{10,25,8,255},{25,8,10,255}}, // seraph
 {0x1u,1,0,0,{10,25,8,255},{25,8,10,255}}, // glaucus
 {0x1u,1,0,0,{255},{255}}, // dwellerDrone
 {0x1u,1,0,0,{255},{255}}, // dwellerGuard
 {0x1u,1,0,0,{255},{255}}, // dwellerWarrior
 {0x1u,1,0,0,{255},{255}}, // null
};
