/*!
 * \file mouse_effects.c
 * \brief The module that draws the graphical effects for the mouse lives here.
 * It was grabbed from Geraldine's Jellybean Machine and existed before I started taking
 * this class, so some bits are probably a bit weird...
 */

#define     NUM_OLD_POINTS              64
#define     NUM_SHOCKRINGS              128
#define     SHOCKRING_START_RADIUS      0.1
#define     SHOCKRING_RADIUS_ADD        1.15
#define     SHOCKRING_MAX_BRIGHTNESS    1.0125
#define     SHOCKRING_MIN_BRIGHTNESS    0.0001
#define     SHOCKRING_MULT              0.9775
#define     CURSOR_IDLE_SIZE            32
#define     CURSOR_CLICK_SIZE           25
#define     CURSOR_OFFSET               (-4)
#define     CURSOR_GFX_PATH             "./gfx/mouse/cursor.tga"
#define     SHOCKRING_GFX_PATH          "./gfx/mouse/shockring.pcx"

#include "client-common.h"
#include "mouse_effects.h"

//! The points used to draw mouse trails.
typedef struct
{
    float x;
    float y;
} OLD_POINT;

//! The shock ring that appears after a click.
typedef struct
{
    float x;
    float y;
    float intensity;
    float radius;
} SHOCK_RING;

/******************************************************************************************/

OLD_POINT mefx_trail_points[NUM_OLD_POINTS];
SHOCK_RING mefx_shockring_pool[NUM_SHOCKRINGS];

/*! defgroup mefx_priv Private data for the mouse-effects module.
 * \{ */
static int mefx_curr_point             = 0;
static int mefx_curr_shockring         = 0;
static TEXTURE_HANDLE mefx_cursor_gfx          = 0;
static TEXTURE_HANDLE mefx_shocrking_gfx       = 0;
static int mefx_cursor_size            = CURSOR_IDLE_SIZE;
/*! \} */

/******************************************************************************************/
/*! @brief Load all required assets for this module
 */
void MEFX_load(void)
{
    if(!exists(CURSOR_GFX_PATH))
        OH_SMEG("Missing asset: %s",CURSOR_GFX_PATH);
    mefx_cursor_gfx = COMMON_load_texture(CURSOR_GFX_PATH);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    if(!exists(SHOCKRING_GFX_PATH))
        OH_SMEG("Missing asset: %s", SHOCKRING_GFX_PATH);
    mefx_shocrking_gfx = COMMON_load_texture(SHOCKRING_GFX_PATH);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}


/******************************************************************************************/
/*! @brief Discard loaded assets, free memory.
 */
void MEFX_unload(void)
{
    glDeleteTextures(1, &mefx_cursor_gfx);
    glDeleteTextures(1, &mefx_shocrking_gfx);
}

/******************************************************************************************/
/*! @brief Reset all the mouse effects.
 */
void MEFX_reset(void)
{
    int index;

    for(index = 0; index < NUM_OLD_POINTS; index++)
    {
        mefx_trail_points[index].x = mouse_x;
        mefx_trail_points[index].y = mouse_y;
    }

    for(index = 0; index < NUM_SHOCKRINGS; index++)
        mefx_shockring_pool[index].intensity = 0.0f;
}

/******************************************************************************************/
/*! @brief Start a shock ring effect at the specified coordinates.  The rings are
 *  handled automatically after they're spawned.
 *
 *  @param x The horizontal position to place the ring at.
 *  @param y The vertical position to place the ring at.
 */
void MEFX_spawn_shockring(void)
{
    mefx_shockring_pool[mefx_curr_shockring].x = mouse_x;
    mefx_shockring_pool[mefx_curr_shockring].y = mouse_y;
    mefx_shockring_pool[mefx_curr_shockring].radius = SHOCKRING_START_RADIUS;
    mefx_shockring_pool[mefx_curr_shockring].intensity = SHOCKRING_MAX_BRIGHTNESS;
    mefx_curr_shockring++;

    if(mefx_curr_shockring >= NUM_SHOCKRINGS)
        mefx_curr_shockring = 0;

    mefx_cursor_size = CURSOR_CLICK_SIZE;
}

/******************************************************************************************/
/*! @brief Advance all mouse effect animations by one frame.
 */
void MEFX_tick(void)
{
    int index;

    // update cursor click anim
    if(mefx_cursor_size < CURSOR_IDLE_SIZE)
        mefx_cursor_size++;

    // update shockrings
    for(index = 0; index < NUM_SHOCKRINGS; index++)
    {
        if(mefx_shockring_pool[index].intensity > SHOCKRING_MIN_BRIGHTNESS)
        {
            mefx_shockring_pool[index].radius += SHOCKRING_RADIUS_ADD;
            mefx_shockring_pool[index].intensity *= SHOCKRING_MULT;
        }
    }

    // update trails
    mefx_trail_points[mefx_curr_point].x = mouse_x;
    mefx_trail_points[mefx_curr_point].y = mouse_y;

    mefx_curr_point++;

    if(mefx_curr_point > NUM_OLD_POINTS - 1) mefx_curr_point = 0;
}

/******************************************************************************************/
/*! @brief Draw all effects.
 */
void MEFX_draw(void)
{
    int index;
    int vert_index = 0;
    float col_divisor = getr(makecol(255, 255, 255));
    static int trail_hue;

    VERTEX      trail_verts[NUM_OLD_POINTS];
    VERTEX      shock_verts[5];
    TEX_COORD   shock_uvs[5];

    COLOR trail_colours[NUM_OLD_POINTS];

    // advance the trail colour cycling
    trail_hue+=3;

    // temp variables for computing hsv->rgb
    int r_tmp, g_tmp, b_tmp;

    // for dimming the portions of the trail further from the current point
    float brightness_tmp;

    // walk the points array and draw coloured, translucent lines between them
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_COLOR, GL_ONE);

    for(index = mefx_curr_point; index < mefx_curr_point + (NUM_OLD_POINTS); index++)
    {
        brightness_tmp = (float)(index - mefx_curr_point) / (float)NUM_OLD_POINTS;
        hsv_to_rgb((trail_hue + (index - mefx_curr_point)) % 360,   // hue
            1.0 - (brightness_tmp / 1.25),                          // saturation
            brightness_tmp,                                         // value
            &r_tmp, &g_tmp, &b_tmp);

        trail_verts[vert_index].x   = mefx_trail_points[index % NUM_OLD_POINTS].x;
        trail_verts[vert_index].y   = mefx_trail_points[index % NUM_OLD_POINTS].y;
        trail_verts[vert_index].z   = 0.0f;
        trail_colours[vert_index].r = r_tmp / col_divisor;
        trail_colours[vert_index].g = g_tmp / col_divisor;
        trail_colours[vert_index].b = b_tmp / col_divisor;

        vert_index++;
    }

    glVertexPointer(3, GL_FLOAT, 0, trail_verts);
    glColorPointer(4, GL_FLOAT, 0, trail_colours);
    glDrawArrays(GL_LINE_STRIP, 0, NUM_OLD_POINTS);

    // draw any outstanding shock rings
    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mefx_shocrking_gfx);
    glVertexPointer(3, GL_FLOAT, 0, shock_verts);
    glTexCoordPointer(2, GL_FLOAT, 0, shock_uvs);

    // 07/03/13 23:14
    // TODO: don't compute this every frame...
    shock_uvs[0].u = 0.0f;
    shock_uvs[0].v = 0.0f;

    shock_uvs[1].u = 1.0f;
    shock_uvs[1].v = 0.0f;

    shock_uvs[2].u = 1.0f;
    shock_uvs[2].v = 1.0f;

    shock_uvs[3].u = 0.0f;
    shock_uvs[3].v = 1.0f;

    for(index = 0; index < NUM_SHOCKRINGS; index++)
    {
        // walk the array, and draw translucent circles for any 'alive' shock rings
        if(mefx_shockring_pool[index].intensity > SHOCKRING_MIN_BRIGHTNESS)
        {
            // brightness
            glColor4f(mefx_shockring_pool[index].intensity, mefx_shockring_pool[index].intensity,
                mefx_shockring_pool[index].intensity, 1.0f);

            // size
            shock_verts[0].x = mefx_shockring_pool[index].x - mefx_shockring_pool[index].radius;
            shock_verts[0].y = mefx_shockring_pool[index].y - mefx_shockring_pool[index].radius;
            shock_verts[0].z = 0;

            shock_verts[1].x = mefx_shockring_pool[index].x + mefx_shockring_pool[index].radius;
            shock_verts[1].y = mefx_shockring_pool[index].y - mefx_shockring_pool[index].radius;
            shock_verts[1].z = 0;

            shock_verts[2].x = mefx_shockring_pool[index].x + mefx_shockring_pool[index].radius;
            shock_verts[2].y = mefx_shockring_pool[index].y + mefx_shockring_pool[index].radius;
            shock_verts[2].z = 0;

            shock_verts[3].x = mefx_shockring_pool[index].x - mefx_shockring_pool[index].radius;
            shock_verts[3].y = mefx_shockring_pool[index].y + mefx_shockring_pool[index].radius;
            shock_verts[3].z = 0;

            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
    }

    // draw the pointing hand
    int point_to_track =  mefx_curr_point - 1;
    if (point_to_track < 0) point_to_track = NUM_OLD_POINTS - 1;
    glBindTexture(GL_TEXTURE_2D, mefx_cursor_gfx);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    shock_verts[0].x = mefx_trail_points[point_to_track].x + CURSOR_OFFSET;
    shock_verts[0].y = mefx_trail_points[point_to_track].y + CURSOR_OFFSET;
    shock_verts[0].z = -1;

    shock_verts[1].x = mefx_trail_points[point_to_track].x + mefx_cursor_size + CURSOR_OFFSET;
    shock_verts[1].y = mefx_trail_points[point_to_track].y + CURSOR_OFFSET;
    shock_verts[1].z = -1;

    shock_verts[2].x = mefx_trail_points[point_to_track].x + mefx_cursor_size + CURSOR_OFFSET;
    shock_verts[2].y = mefx_trail_points[point_to_track].y + mefx_cursor_size + CURSOR_OFFSET;
    shock_verts[2].z = -1;

    shock_verts[3].x = mefx_trail_points[point_to_track].x + CURSOR_OFFSET;
    shock_verts[3].y = mefx_trail_points[point_to_track].y + mefx_cursor_size + CURSOR_OFFSET;
    shock_verts[3].z = -1;

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisable(GL_BLEND);
}


