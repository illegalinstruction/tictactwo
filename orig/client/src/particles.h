#include    "client-common.h"
#ifndef     PARTICLES_H
   #define  PARTICLES_H

    #define  NUM_PARTICLE_COLOURS        6
    #define  MAX_PARTICLES               1024
    #define  MAX_CONFETTI                1024

    #define  PARTICLE_CONFETTI           1
    #define  PARTICLE_HEART              2
    #define  PARTICLE_SPARK              3

    void PARTICLE_init(void);
    void PARTICLE_load(void);
    void PARTICLE_tick(void);
    void PARTICLE_draw(void);
    void PARTICLE_unload(void);
    void PARTICLE_spawn(float x, float y, float x_speed, float y_speed, int type, int intensity, int colour);
#endif
