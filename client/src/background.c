/*! \file background.c
 * \todo include a bit of glsl that does a convolution blur based on distance
 * \todo chromatic aberration (3 passes, each with red/grn/blue masked off)
 */
#include "background.h"
#include "ms3d_model.h"

#define     BACKGROUND_TUNNEL_GEOM_PATH     "./gfx/background/tunnel.ms3d"
#define     BACKGROUND_TUNNEL_TEX1_PATH     "./gfx/background/tunnel1.pcx"
#define     BACKGROUND_TUNNEL_TEX2_PATH     "./gfx/background/tunnel2.pcx"

#define     BACKGROUND_ZERO_TEX_PATH        "./gfx/background/zero.pcx"
#define     BACKGROUND_ONE_TEX_PATH         "./gfx/background/one.pcx"

#define     BACKGROUND_STAR_SPEED           0.35f
#define     BACKGROUND_NUM_STARS            165

#define     BACKGROUND_STAR_MAX_X           100.0f
#define     BACKGROUND_STAR_MAX_Z           10.0f
#define     BACKGROUND_STAR_MAX_Y           10.0f

#define     BACKGROUND_FOG_DIST             60.0f

/*! \brief A structure that describes the position and brightness of a '1'/'0' sprite */
typedef struct
{
    VERTEX      pos;
    int         time_to_live;
    int         type;
} BACKGROUND_STAR;

/****************************************************************************************************************/
/*! \defgroup background_priv "Background module private stuff"
 * \brief Private data and functions required to display the background art.
 * \{ */

static MS3D_MODEL       *bkgrnd_tunnel_geom = NULL;
static TEXTURE_HANDLE   bkgrnd_tunnel_tex1;
static TEXTURE_HANDLE   bkgrnd_tunnel_tex2;
static TEXTURE_HANDLE   bkgrnd_sprite_one_tex;
static TEXTURE_HANDLE   bkgrnd_sprite_zero_tex;
static BOOL             bkgrnd_was_inited = FALSE;
static uint32_t         bkgrnd_anim_clock = 0;

static BACKGROUND_STAR  bkgrnd_stars[BACKGROUND_NUM_STARS];

/*! \brief Helper function to animate the 'stars' in the background. */
static void             bkgrnd_star_tick(void);

void bkgrnd_unload(void);

/*! \} */

/****************************************************************************************************************/
/*! \brief Load all required art and prepare the background animation to be drawn.
 */
void BKGRND_load(void)
{
    if (exists(BACKGROUND_TUNNEL_GEOM_PATH))
    {
        bkgrnd_tunnel_geom = MS3D_load(BACKGROUND_TUNNEL_GEOM_PATH);
    }
    else
    {
        OH_SMEG("Missing asset - %s. The background almost certainly won't look right.",BACKGROUND_TUNNEL_GEOM_PATH);
        bkgrnd_tunnel_geom = NULL;
    }

    bkgrnd_tunnel_tex1 = COMMON_load_texture(BACKGROUND_TUNNEL_TEX1_PATH);
    bkgrnd_tunnel_tex2 = COMMON_load_texture(BACKGROUND_TUNNEL_TEX2_PATH);

    bkgrnd_sprite_one_tex   = COMMON_load_texture(BACKGROUND_ZERO_TEX_PATH);
    bkgrnd_sprite_zero_tex  = COMMON_load_texture(BACKGROUND_ONE_TEX_PATH);

    int index;

    for (index = 0; index < BACKGROUND_NUM_STARS; index++)
    {
        bkgrnd_stars[index].pos.x = rand() % (int)BACKGROUND_STAR_MAX_X;
        bkgrnd_stars[index].pos.y = rand() % (int)BACKGROUND_STAR_MAX_Y;
        bkgrnd_stars[index].pos.z = rand() % (int)BACKGROUND_STAR_MAX_Z;
        bkgrnd_stars[index].time_to_live = rand() % 256;
        bkgrnd_stars[index].type        = rand() % 2;
    }

    float fog_dist = BACKGROUND_FOG_DIST;

    glFogfv(GL_FOG_END, &fog_dist);

    atexit(bkgrnd_unload);
}

/****************************************************************************************************************/
/*! \brief Animate the stars in the background.
 */
static void bkgrnd_star_tick(void)
{
    int index;

    for(index = 0; index < BACKGROUND_NUM_STARS; index++)
    {
        bkgrnd_stars[index].pos.x += BACKGROUND_STAR_SPEED;
        bkgrnd_stars[index].time_to_live--;

        if (bkgrnd_stars[index].time_to_live < 1)
        {
            bkgrnd_stars[index].pos.x = (rand() % (int)(BACKGROUND_STAR_MAX_X * 2)) - BACKGROUND_STAR_MAX_X;
            bkgrnd_stars[index].pos.y = (rand() % (int)(BACKGROUND_STAR_MAX_Y * 2)) - BACKGROUND_STAR_MAX_Y;
            bkgrnd_stars[index].pos.z = (rand() % (int)(BACKGROUND_STAR_MAX_Z * 2)) - BACKGROUND_STAR_MAX_Z;
            bkgrnd_stars[index].time_to_live = 255;
        }
    }
}

/****************************************************************************************************************/
/*! \brief Animate the stars in the background.
 */
void BKGRND_draw(void)
{
    COMMON_to_perspective();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_FOG);

    int r_tmp;
    int g_tmp;
    int b_tmp;

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glColor3ub( (int)((sinf(bkgrnd_anim_clock / 65.0f) * 127) + 128),
                (int)((sinf(bkgrnd_anim_clock / 74.0f) * 127) + 128),
                (int)((sinf(bkgrnd_anim_clock / 59.0f) * 127) + 128));

    glLoadIdentity();

    COMMON_look_at( sinf(bkgrnd_anim_clock / 98.0f) * 10.0f,
                    sinf(bkgrnd_anim_clock / 109.1f) * 3.0f,
                    (sinf(bkgrnd_anim_clock / 176.3f) * 2.0f) + (sinf(bkgrnd_anim_clock / 59.3f) * 1.4f) ,
                    0, 0, 0);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    // this controls the scrolling of the texture; we don't animate the actual geometry at all
    glTranslatef((sinf(bkgrnd_anim_clock / 79.1f) / 3.0f) + (sinf(bkgrnd_anim_clock / 121.1f) / 2.125f),
        bkgrnd_anim_clock / 323.4f, 0);
    glMatrixMode(GL_MODELVIEW);

    // base tunnel texture
    glDisable(GL_BLEND);
    MS3D_draw(bkgrnd_tunnel_geom, bkgrnd_tunnel_tex1, 0, 0, 0, 0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    // this controls the scrolling of the texture; we don't animate the actual geometry at all
    glTranslatef(sinf(bkgrnd_anim_clock / 143.1f) / 1.25,
        bkgrnd_anim_clock / 125.0f + (sinf(bkgrnd_anim_clock / 79.1f) / 21.0f) , 0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_COLOR, GL_ONE);

    // colour pattern of the blended texture
    glColor3ub( (int)((cosf(bkgrnd_anim_clock / 65.0f) * 127) + 128),
                (int)((sinf(bkgrnd_anim_clock / 71.3f) * 127) + 128),
                (int)((cosf(bkgrnd_anim_clock / 109.0f) * 127) + 128));

    // blended tunnel texture
    MS3D_draw(bkgrnd_tunnel_geom, bkgrnd_tunnel_tex2, 0, 0, 0, 0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    // already in correct blend mode, draw the stars
    int index;

    for(index = 0; index < BACKGROUND_NUM_STARS; index++)
    {
        if (bkgrnd_stars[index].time_to_live > 127)
        {
            glColor3ub( (255 - bkgrnd_stars[index].time_to_live) * 2,
                        (255 - bkgrnd_stars[index].time_to_live) * 2,
                        (255 - bkgrnd_stars[index].time_to_live) * 2);
        }
        else
        {
            glColor3ub( bkgrnd_stars[index].time_to_live * 2,
                        bkgrnd_stars[index].time_to_live * 2,
                        bkgrnd_stars[index].time_to_live * 2);
        }

        switch (bkgrnd_stars[index].type)
        {
            case 0:
                COMMON_draw_sprite(bkgrnd_sprite_zero_tex,
                    bkgrnd_stars[index].pos.x, bkgrnd_stars[index].pos.y, bkgrnd_stars[index].pos.z, 2, 2);
            break;

            default:
                COMMON_draw_sprite(bkgrnd_sprite_one_tex,
                    bkgrnd_stars[index].pos.x, bkgrnd_stars[index].pos.y, bkgrnd_stars[index].pos.z, 2, 2);
            break;
        }
    }

    glDisable(GL_BLEND);
    glColor3ub(255, 255, 255);

    COMMON_to_ortho();
}

/****************************************************************************************************************/
/*! \brief Advance the background animation by exactly one logic tick.
 */
void BKGRND_tick(void)
{
    bkgrnd_star_tick();
    bkgrnd_anim_clock++;
}

/****************************************************************************************************************/
/*! \brief Unload any assets and free up memory.  Called automatically upon exit.
 */
void bkgrnd_unload(void)
{
    MS3D_unload(bkgrnd_tunnel_geom);
    glDeleteTextures(1, &bkgrnd_tunnel_tex1);
    glDeleteTextures(1, &bkgrnd_tunnel_tex2);
    glDeleteTextures(1, &bkgrnd_sprite_one_tex);
    glDeleteTextures(1, &bkgrnd_sprite_zero_tex);
}
