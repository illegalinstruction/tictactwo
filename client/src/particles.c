#include "particles.h"

#define     CONFETTI_GRAV_ACCELERATION  0.025
#define     CONFETTI_INTENSITY_LOSS     4

#define     HEART_INTENSITY_LOSS        2
#define     HEART_SPEED_ATTENUATION     0.9675

#define     MAX_SLOTS_TO_WALK           64

#define     PRT_HEART_GFX_PATH          "./gfx/particle/heart/%02d.tga"
#define     PRT_SPARK_GFX_PATH          "./gfx/particle/spark/%02d.tga"
#define     PRT_CONFETTI_GFX_PATH       "./gfx/particle/confetti.tga"

#define     PRT_HEART_SIZE              36
#define     PRT_SPARK_SIZE              38
#define     PRT_CONFETTI_SIZE           20

/******************************************************************************************/

typedef struct PARTICLE_STRUCT
{
    float   x;
    float   y;
    float   x_speed;
    float   y_speed;
    int     intensity;
    int     colour;
} PARTICLE;

PARTICLE    prtcl_spark_pool[MAX_PARTICLES];
PARTICLE    prtcl_heart_pool[MAX_PARTICLES];
PARTICLE    prtcl_confetti_pool[MAX_CONFETTI];

GLuint      prtcl_spark_gfx[NUM_PARTICLE_COLOURS];
GLuint      prtcl_heart_gfx[NUM_PARTICLE_COLOURS];
GLuint      prtcl_confetti_gfx;

/******************************************************************************************/
/*!
 * @brief Document me, please.
 */
void PARTICLE_init(void)
{
    int index;

    for(index = 0; index < MAX_PARTICLES; index++)
    {
        prtcl_spark_pool[index].intensity       = 0;
        prtcl_heart_pool[index].intensity       = 0;
    }

    for(index = 0; index < MAX_CONFETTI; index++)
    {
        prtcl_confetti_pool[index].intensity    = 0;
    }
}

/******************************************************************************************/
/*!
 * @brief Document me too, please. Explain what I do if assets are missing.
 */
void PARTICLE_load(void)
{
    int index;
    char buf[1024];

    for(index = 0; index < NUM_PARTICLE_COLOURS; index++)
    {
        snprintf(buf, 1023, PRT_HEART_GFX_PATH, index + 1);
        prtcl_heart_gfx[index] = COMMON_load_texture(buf);

        snprintf(buf, 1023, PRT_SPARK_GFX_PATH, index + 1);
        prtcl_spark_gfx[index] = COMMON_load_texture(buf);
    }

    prtcl_confetti_gfx = COMMON_load_texture(PRT_CONFETTI_GFX_PATH);
}

/******************************************************************************************/
/*!
 * @brief Unload all assets and free up memory.
 */
void PARTICLE_unload(void)
{
    int index;

    for(index = 0; index < NUM_PARTICLE_COLOURS; index++)
    {
        glDeleteTextures(1, &prtcl_heart_gfx[index]);
        glDeleteTextures(1, &prtcl_spark_gfx[index]);
    }

    glDeleteTextures(1, &prtcl_confetti_gfx);
}

/******************************************************************************************/
/*!
 * @brief Advance all particle animations by one frame.
 */
void PARTICLE_tick(void)
{
    int index;

    for(index = 0; index < MAX_PARTICLES; index++)
    {
        // handle confetti
        if(prtcl_confetti_pool[index].intensity > 0)
        {
            prtcl_confetti_pool[index].intensity -= CONFETTI_INTENSITY_LOSS;
            prtcl_confetti_pool[index].y_speed += CONFETTI_GRAV_ACCELERATION;

            prtcl_confetti_pool[index].x += prtcl_confetti_pool[index].x_speed;
            prtcl_confetti_pool[index].y += prtcl_confetti_pool[index].y_speed;
        }

        // handle hearts
        if(prtcl_heart_pool[index].intensity > 0)
        {
            prtcl_heart_pool[index].intensity -= HEART_INTENSITY_LOSS;
            prtcl_heart_pool[index].x_speed *= HEART_SPEED_ATTENUATION;
            prtcl_heart_pool[index].y_speed *= HEART_SPEED_ATTENUATION;

            prtcl_heart_pool[index].x += prtcl_heart_pool[index].x_speed;
            prtcl_heart_pool[index].y += prtcl_heart_pool[index].y_speed;
        }

        // handle sparks
        if(prtcl_spark_pool[index].intensity > 0)
        {
            prtcl_spark_pool[index].intensity -= HEART_INTENSITY_LOSS;
            prtcl_spark_pool[index].x_speed *= HEART_SPEED_ATTENUATION;
            prtcl_spark_pool[index].y_speed *= HEART_SPEED_ATTENUATION;

            prtcl_spark_pool[index].x += prtcl_spark_pool[index].x_speed;
            prtcl_spark_pool[index].y += prtcl_spark_pool[index].y_speed;
        }
    }

}

/******************************************************************************************/
/*!
 * @brief Try to spawn a particle of the requested type.  If we can't find an empty slot
 *   to spawn it in in a reasonable amount of time, we fail silently.
 */
void PARTICLE_spawn(float x, float y, float x_speed, float y_speed, int type, int intensity, int colour)
{
    static int heart_index;
    static int spark_index;
    static int confetti_index;
    int num_slots_walked = 0;

    switch(type)
    {
        case PARTICLE_CONFETTI:
            while (num_slots_walked < MAX_SLOTS_TO_WALK)
            {
                if (prtcl_confetti_pool[confetti_index].intensity < 1)
                {
                    // found an empty slot; use it.
                    prtcl_confetti_pool[confetti_index].x           = x;
                    prtcl_confetti_pool[confetti_index].y           = y;
                    prtcl_confetti_pool[confetti_index].x_speed     = x_speed;
                    prtcl_confetti_pool[confetti_index].y_speed     = y_speed;
                    prtcl_confetti_pool[confetti_index].intensity   = intensity;

                    // move the index to the next likely-to-be-empty slot.
                    confetti_index++;
                    if (confetti_index > (MAX_CONFETTI - 1))
                        confetti_index = 0;

                    return;
                }

                // keep walking the pool
                confetti_index++;
                if (confetti_index > (MAX_CONFETTI - 1))
                    confetti_index = 0;

                // how many slots have we walked past? if too many,
                // we give up so we don't spend forever searching.
                num_slots_walked++;
            }
        break;

        //////////////////////////////////////////////////////////////////

        case PARTICLE_HEART:
            while (num_slots_walked < MAX_SLOTS_TO_WALK)
            {
                if (prtcl_heart_pool[heart_index].intensity < 1)
                {
                    prtcl_heart_pool[heart_index].x           = x;
                    prtcl_heart_pool[heart_index].y           = y;
                    prtcl_heart_pool[heart_index].x_speed     = x_speed;
                    prtcl_heart_pool[heart_index].y_speed     = y_speed;
                    prtcl_heart_pool[heart_index].intensity   = intensity;
                    prtcl_heart_pool[heart_index].colour      = colour;
                    heart_index++;

                    if (heart_index > (MAX_PARTICLES - 1))
                        heart_index = 0;

                    return;
                }

                heart_index++;
                if (heart_index > (MAX_PARTICLES - 1))
                    heart_index = 0;

                num_slots_walked++;
            }
        break;

        //////////////////////////////////////////////////////////////////

        case PARTICLE_SPARK:
            while (num_slots_walked < MAX_SLOTS_TO_WALK)
            {
                if (prtcl_spark_pool[spark_index].intensity < 1)
                {
                    prtcl_spark_pool[spark_index].x           = x;
                    prtcl_spark_pool[spark_index].y           = y;
                    prtcl_spark_pool[spark_index].x_speed     = x_speed;
                    prtcl_spark_pool[spark_index].y_speed     = y_speed;
                    prtcl_spark_pool[spark_index].intensity   = intensity;
                    prtcl_spark_pool[spark_index].colour      = colour;
                    spark_index++;

                    if (spark_index > (MAX_PARTICLES - 1))
                        spark_index = 0;

                    return;
                }

                spark_index++;
                if (spark_index > (MAX_PARTICLES - 1))
                    spark_index = 0;

                num_slots_walked++;
            }
        break;

        //////////////////////////////////////////////////////////////////

        default:
            DUH_WHERE_AM_I("\x1b[31mcalled with invalid value %d :o( \x1b[0m", type);
    }

    // couldn't find an empty within the allotted number of
    // tries; give up.
    return;
}

/******************************************************************************************/
/*!
 * @brief Draw all active particles.
 */
void PARTICLE_draw(void)
{
    int index;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_COLOR, GL_ONE);

    for(index = 0; index < MAX_PARTICLES; index++)
    {
        // handle confetti
        if(prtcl_confetti_pool[index].intensity > 0)
        { /*
            glColor3ub(prtcl_confetti_pool[index].intensity, prtcl_confetti_pool[index].intensity, prtcl_confetti_pool[index].intensity, );

            COMMON_draw_sprite(

            draw_trans_sprite(double_buffer,
                prtcl_confetti_gfx[prtcl_confetti_pool[index].intensity % 3],
                (int)prtcl_confetti_pool[index].x,
                (int)prtcl_confetti_pool[index].y); */
        }

        // handle hearts
        if(prtcl_heart_pool[index].intensity > 0)
        {
            glColor3ub(prtcl_heart_pool[index].intensity,prtcl_heart_pool[index].intensity,prtcl_heart_pool[index].intensity);

            COMMON_draw_sprite(prtcl_heart_gfx[prtcl_heart_pool[index].colour],
                prtcl_heart_pool[index].x, prtcl_heart_pool[index].y, 0.5, PRT_HEART_SIZE, PRT_HEART_SIZE);
        }

        // handle sparks
        if(prtcl_spark_pool[index].intensity > 0)
        {
            glColor3ub(prtcl_spark_pool[index].intensity, prtcl_spark_pool[index].intensity, prtcl_spark_pool[index].intensity);

            COMMON_draw_sprite(prtcl_spark_gfx[prtcl_spark_pool[index].colour],
                prtcl_spark_pool[index].x, prtcl_spark_pool[index].y, 0.5, PRT_SPARK_SIZE, PRT_SPARK_SIZE);
        }
    }
}
