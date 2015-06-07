#include "client-common.h"

#define HOME_PATH_LENGTH        4096
#define PRINTF_BUF_LENGTH       128

/*! \brief For network-aware apps, the TCP connection to the server. Kept private - client code should user
 * COMMON_connect(), COMMON_send(), COMMON_recv() and COMMON_disconnect(). */
static int common_connection_fd = -1;

/*! \defgroup dumb_priv_data
 * \brief Module-player management stuff; you really shouldn't need to worry about this at all.
 * \{ */
static DUH             *common_duh             = NULL;
static AL_DUH_PLAYER   *common_duhplyr         = NULL;
static float           common_duhvol           = DUMB_VOL;
static int             common_curr_bgm         = 0;
static int             common_should_fade_bgm;
/*! \} */

volatile int    common_frames_this_sec;
volatile int    common_frames_per_sec;

/*! \defgroup look_at_helpers
 * \brief Variables to keep track of which way the camera is facing.
 * \{ */
static float common_x_rot;
static float common_y_rot;
/*! \} */

static float common_aspect_ratio;
static TEXTURE_HANDLE texture_name = 1;

/*! \brief The default font texture for the application. */
GLuint common_gamefont;

int common_curr_state;
BOOL common_curr_state_done;
int common_next_state;
int common_next_state_param;

/*! \brief The width of the display in virtual coordinates, adjusted by the autocomputed aspect ratio */
float common_effective_display_width;
/*! \brief A string representing the path to the home directory of the user the game is running as. */
char common_user_home_path[HOME_PATH_LENGTH];

/****************************************************************************************************************/
/*! \brief If the background music was asked to fade out, this will remain false until
 *  the volume is too low to be heard, at which point, it'll return true. Useful for
 *  syncing screen fades, etc. to the music fading out.
 */
int             common_bgm_fade_done    = FALSE;

/****************************************************************************************************************/
/*! \brief Reports how many frames we were able to draw in the last second.
 * It is bound to a timer and should never be called manually.
 */
void sample_framerate(void)
{
    common_frames_per_sec  = common_frames_this_sec;
    common_frames_this_sec = 0;
}
/*! \cond PRIVATE */
END_OF_FUNCTION(sample_framerate);
/*! \endcond */

/****************************************************************************************************************/
/*!
 * \brief Advances the game logic by exactly one tick, even if repainting the current frame
 * ends up taking more than the allotted time. Called automatically as a timer task and should
 * <i>never</i> be called manually.
 *
 * \note Please see the blurb at common_time_for_logic for how to respond to logic updates.
 */
void game_heartbeat()
{
    common_time_for_logic++;
}
/*! \cond PRIVATE */
END_OF_FUNCTION(game_heartbeat);
/*! \endcond */

/****************************************************************************************************************/
/*! \brief Tracks whether the application is currently using a perspective (3D) or orthographic
 *  (2D) projection.
 *
 *  \note Will be true if perspective is enabled, false otherwise.
 */
BOOL common_persp_mode = FALSE;

/****************************************************************************************************************/
/*! \brief This is updated automagically in a separate thread and will remain zero until
 *  it's time to run the game's next logic tick.
 */
volatile int common_time_for_logic;

/****************************************************************************************************************/
/*! @brief Tries to connect to a remote machine with the specified address and port number.
 * @return TRUE if successful, FALSE if it fails for any reason.
 */
BOOL COMMON_connect(uint32_t remote_address, uint16_t remote_port)
{
    struct sockaddr_in tmp;
    tmp.sin_family      = AF_INET;
    tmp.sin_addr.s_addr = remote_address;
    tmp.sin_port        = htons(remote_port);

    common_connection_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(common_connection_fd, (struct sockaddr *)&tmp, sizeof(tmp)) == -1)
    {
        // failed
        DUH_WHERE_AM_I("Unable to connect...");
        close(common_connection_fd);
        return FALSE;
    }

    // only set this to nonblocking after we have it
    fcntl(common_connection_fd, F_SETFL, O_NONBLOCK);

    DUH_WHERE_AM_I("Successfully connected to %d on port %d", remote_address, remote_port);
    return TRUE;
}

/****************************************************************************************************************/
/*!
 * @brief Sends out a chunk of data across the network. The connection has to have been opened
 * already.
 */
BOOL COMMON_send(void *data, uint16_t length)
{
    if (common_connection_fd == -1)
        return FALSE;

    int bytes_sent = send(common_connection_fd, data, length, MSG_DONTWAIT);

    if (bytes_sent != length)
        return FALSE;

    return TRUE;
}

/****************************************************************************************************************/
/*!
 * @brief Grabs an incoming chunk of data if one is available. The connection has to have been
 * opened already.
 */
BOOL COMMON_recv(void *data, uint16_t length)
{
    if (common_connection_fd == -1)
        return FALSE;

    if (recv(common_connection_fd, data, length, MSG_DONTWAIT) == -1)
    {
        return FALSE;
    }

    return TRUE;
}

/****************************************************************************************************************/
/*!
 * @brief Closes a connection, if there's one open, or fails silently if not.
 */
void COMMON_disconnect(void)
{
    if (common_connection_fd != -1)
        close(common_connection_fd);

    common_connection_fd = -1;
}

/****************************************************************************************************************/
/*! \brief Change what music is playing. To stop BGM entirely, pass a non-existant
 * track ID (usually 0 if we've set up our bgm/ directory right).
 *
 * \note After asking for a track change, you shouldn't need to worry about it anymore; the
 * state of the BGM player is polled for you automatically in COMMON_flip_buffer().
 */
void COMMON_set_bgm(int track)
{
    char buffer[2048];

    // already playing this, don't need to do anything
    if(track == common_curr_bgm) return;

    common_curr_bgm = track;

    // check whether requested music exists
    sprintf(buffer,"./bgm/%02d.xm",track);
    if(exists(buffer))
    {
        al_stop_duh(common_duhplyr);

        // yes, load it and start playing
        common_duh             = dumb_load_xm_quick(buffer);
        common_duhplyr         = al_start_duh(common_duh, 2, 0, DUMB_VOL, DUMB_BUFSIZE, DUMB_MIXRATE);
        common_bgm_fade_done   = FALSE;
        common_duhvol          = DUMB_VOL;
        common_should_fade_bgm = FALSE;
    }
    else
    {
        // no - null out duh and player
        al_stop_duh(common_duhplyr);
        common_duhplyr = NULL;
        common_bgm_fade_done = TRUE; // not technically, but we _are_ silent at this point...
    }
}

/****************************************************************************************************************/
/*! \brief Cause the currently-playing track to fade out.
 *  Returns immediately and does not block; the actual act of fading happens
 *  in COMMON_flip_buffer.
 */
void COMMON_fade_bgm(void)
{
    common_should_fade_bgm = TRUE;
}

/****************************************************************************************************************/
/*! \brief Get the track ID of the currently-playing BGM.
 */
int COMMON_get_bgm(void)
{
    return common_curr_bgm;
}

/****************************************************************************************************************/
/*!
 * \defgroup Text Output Macros
 * \brief These control how a texture font is processed; microgame implementers
 * shouldn't need to worry about them, and artists may safely assume that the
 * font renderer thinks of all glyphs as being square, always arranged 16 glyphs
 * to a row, and the first glyph being ASCII #32 (a space).
 *
 * \note The widths and heights of each glyph are fractions of a texture
 * coordinate, rather than absolute pixel values.
 *
 * @{
 */
#define GLYPHS_PER_ROW          16
#define GLYPH_WIDTH             0.0625f
#define GLYPH_HEIGHT            0.125f
#define GLYPH_SCREEN_WIDTH      16.0f
#define FIRST_GLYPH             32
/*! @} */

/****************************************************************************************************************/
/*! \brief Display a string at the specified location, in the specified font.
 *
 * \note Accepts most formatting commands that could be used in arguments to printf();
 * \\r, \\n and \\t are automatically stripped and replaced with spaces.<br><br>
 *
 * \note There isn't any support for characters outside of ASCII currently.<br><br>
 *
 * \note Due to the fact that the string has to be reprocessed internally, can't be used
 * for extremely long texts. (Ideally, you wouldn't want to do this anyway - the entire string
 * has to be walked twice every frame...)<br><br>
 *
 * \note This has no notion of word wrapping or carriage returns.<br><br>
 *
 * \note The drawing colour and blending mode should be set as desired prior to calling this;
 * more often than not, you'll want to use glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA).
 * Also, if texturing isn't enabled when this is called, it almost certainly won't do what you
 * expect.
 * <br><br>
 *
 * \param font The ID of a texture containing a font; passing a non-existent texture isn't a
 * crashable offence, but it will produce really ugly artifacts...
 * \param x The x position to draw the passed-in string at in 3D space.
 * \param y The y position to draw the passed-in string at in 3D space.
 * \param z The z position to draw the passed-in string at in 3D space.  When drawing in an
 * orthographic mode, this has no effect on sorting or occlusion unless you've turned on depth
 * testing, and should usually be set to 0.
 * \param size The width and height each glyph should be; in orthographic modes, the size
 * can be safely treated as if it were in pixels.
 * \param spacing Controls the <i>kerning</i> or spacing between the glyphs; currently,
 * proportional kerning isn't supported.
 * \param str The string to be printed.
 */
void COMMON_glprint(TEXTURE_HANDLE font, GLfloat x, GLfloat y, GLfloat z, GLfloat size, GLfloat spacing,
    const char *str, ...)
{
    int         row;
    int         col;
    int         index;
    VERTEX      verts[4];
    TEX_COORD   uvs[4];
    va_list     variadic_args;
    char        buf[PRINTF_BUF_LENGTH];

    // Null string? Nothing to do here.
    if (str == NULL) return;

    // Process the string's format specifiers and added args...
    va_start(variadic_args, str);
    uvszprintf(buf, PRINTF_BUF_LENGTH-1, str, variadic_args);
    va_end(variadic_args);

    glBindTexture(GL_TEXTURE_2D, font);

    glDisableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glTexCoordPointer(2,GL_FLOAT,0,uvs);
    glVertexPointer(3,GL_FLOAT,0,verts);

    GLfloat centering_offset;

    // Perspective camera?
    if (common_persp_mode)
    {
        // Yes. Centre on the given point, and always turn to
        // face camera.
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(TO_DEGREES(common_y_rot), 0, 1, 0);
        glRotatef(TO_DEGREES(common_x_rot), 1, 0, 0);

        centering_offset = (strlen(str) / 2.0) * (size + spacing);
    }

    for(index = 0; index < strlen(buf); index++)
    {
        // Navigate to the glyph in the texture.
        row = (buf[index] - FIRST_GLYPH) / GLYPHS_PER_ROW;
        col = (buf[index] - FIRST_GLYPH) % GLYPHS_PER_ROW;

        // Are we in 3d?
        if (common_persp_mode)
        {
            // Yes; draw the glyphs such that the string is centered
            // on the given point and rotated to face the camera.
            //
            // Because we treat the orthographic mode as having its
            // origin the top left (OpenGL sort of 'expects' the bottom
            // left by default), each glyph in 3d has to be drawn
            // inverted to fix this up, thus the weird UV coords.
            verts[0].x = -centering_offset + (index * (size + spacing));
            verts[0].y = 0;
            verts[0].z = 0;

            uvs[0].u = col * GLYPH_WIDTH;
            uvs[0].v = (row + 1) * GLYPH_HEIGHT;

            verts[1].x = verts[0].x + size;
            verts[1].y = 0;
            verts[1].z = 0;

            uvs[1].u = (col + 1) * GLYPH_WIDTH;
            uvs[1].v = (row + 1) * GLYPH_HEIGHT;

            verts[2].x = verts[0].x + size;
            verts[2].y = size;
            verts[2].z = 0;

            uvs[2].u = (col + 1) * GLYPH_WIDTH;
            uvs[2].v = row * GLYPH_HEIGHT;

            verts[3].x = verts[0].x;
            verts[3].y = size;
            verts[3].z = 0;

            uvs[3].u = col * GLYPH_WIDTH;
            uvs[3].v = row * GLYPH_HEIGHT;
        }
        else
        {
            // No. Draw these as plain old 2D quads.
            verts[0].x = x  + (index * (size + spacing));
            verts[0].y = y;
            verts[0].z = z;

            uvs[0].u = col * GLYPH_WIDTH;
            uvs[0].v = row * GLYPH_HEIGHT;

            verts[1].x = verts[0].x + size;
            verts[1].y = y;
            verts[1].z = z;

            uvs[1].u = (col + 1) * GLYPH_WIDTH;
            uvs[1].v = row * GLYPH_HEIGHT;

            verts[2].x = verts[0].x + size;
            verts[2].y = y + size;
            verts[2].z = z;

            uvs[2].u = (col + 1) * GLYPH_WIDTH;
            uvs[2].v = (row + 1) * GLYPH_HEIGHT;

            verts[3].x = verts[0].x;
            verts[3].y = y + size;
            verts[3].z = z;

            uvs[3].u = col * GLYPH_WIDTH;
            uvs[3].v = (row + 1) * GLYPH_HEIGHT;
        }

        glDrawArrays(GL_TRIANGLE_FAN,0,4);
    }

    // If we had to modify this, set it back to what the caller's
    // expecting...
    if (common_persp_mode)
        glPopMatrix();
}

/****************************************************************************************************************/
/*!
 * \brief Draws a sprite that always faces the camera.
 *
 * \param sprite The ID of a texture containing the sprite to be drawn.
 * \param x The x position to draw the passed-in texture at in 3D space.
 * \param y The y position to draw the passed-in texture at in 3D space.
 * \param z The z position to draw the passed-in texture at in 3D space.  When drawing in an
 * orthographic mode, this has no effect on sorting or occlusion unless you've turned on depth
 * testing, and should usually be set to 0.
 * \param w The desired width of the sprite.
 * \param h The desired height of the sprite.
 *
 * \note OpenGL has no way of tracking the aspect ratio of its textures, so the width and
 * height are needed; in orthographic mode, the width and height may be safely assumed to
 * be in pixels.
 */
void COMMON_draw_sprite(TEXTURE_HANDLE sprite, float x, float  y, float  z, int w, int h)
{
    VERTEX verts[4];
    TEX_COORD uvs[4];

    glBindTexture(GL_TEXTURE_2D, sprite);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glTexCoordPointer(2,GL_FLOAT,0,uvs);
    glVertexPointer(3,GL_FLOAT,0,verts);

    uvs[0].u = 0;
    uvs[0].v = 0;

    uvs[1].u = 1;
    uvs[1].v = 0;

    uvs[2].u = 1;
    uvs[2].v = 1;

    uvs[3].u = 0;
    uvs[3].v = 1;

    // Perspective camera?
    if (common_persp_mode)
    {
        // Yes. Centre on the given point, and always turn to
        // face camera.
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(TO_DEGREES(common_y_rot), 0, 1, 0);
        glRotatef(TO_DEGREES(common_x_rot), 1, 0, 0);

        verts[0].x = -w / 2.0;
        verts[0].y =  h / 2.0;
        verts[0].z =  0;

        verts[1].x =  w / 2.0;
        verts[1].y =  h / 2.0;
        verts[1].z =  0;

        verts[2].x =  w / 2.0;
        verts[2].y = -h / 2.0;
        verts[2].z =  0;

        verts[3].x = -w / 2.0;
        verts[3].y = -h / 2.0;
        verts[3].z =  0;
    }
    else
    {
        // Orthographic projection, just draw it to screen coords.
        verts[0].x = x;
        verts[0].y = y;
        verts[0].z = z;

        verts[1].x = x + w;
        verts[1].y = y;
        verts[1].z = z;

        verts[2].x = x + w;
        verts[2].y = y + h;
        verts[2].z = z;

        verts[3].x = x;
        verts[3].y = y + h;
        verts[3].z = z;
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // If we had to modify this, set it back to what the caller's
    // expecting...
    if (common_persp_mode)
        glPopMatrix();
}


/****************************************************************************************************************/
/*!
 * \brief Prepare the sound, music, input, timer and graphics subsystems for use.
 * \param use_fullscreen Whether the application should be displayed full-screen or not.
 * \note There is no provision made for switching between windowed and full-screen once the
 * app starts (unfortunately, AllegroGL makes this somewhat tricky and error-prone).
 */
void COMMON_setup(int use_fullscreen)
{
    // Get the path to the user's home directory; there's not really a portable way
    // to do this, so apologies if the following seems inelegant.
    // TODO: look into how this is done on MacOS, which apparently doesn't set HOME...?
#ifdef  __linux__
    strncpy(common_user_home_path, getenv("HOME"), HOME_PATH_LENGTH - 1);
#else
#ifdef _WIN32
    strncpy(common_user_home_path, getenv("USERPROFILE"), HOME_PATH_LENGTH - 1);
#endif
#endif

    // can't believe I forgot this :^/
    srand((unsigned)time(NULL));

    allegro_init();
    install_allegro_gl();
    install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);
    install_keyboard();
    install_mouse();

    // required by DUMB to make sure it dies when the program wants to end and
    // so it can load files.
    atexit(&dumb_exit);
    dumb_register_stdfiles();

    // Set up the display.
    // Compute the correct aspect ratio for the user's display.
    int width_tmp;
    int height_tmp;

    if((!use_fullscreen) || (get_desktop_resolution(&width_tmp, &height_tmp) != 0))
    {
        width_tmp  = DISPLAY_WIDTH;
        height_tmp = DISPLAY_HEIGHT;
    }

    common_aspect_ratio = (float)width_tmp / (float)height_tmp;
    common_effective_display_width =
        DISPLAY_WIDTH * (common_aspect_ratio / ((float)DISPLAY_WIDTH / (float)DISPLAY_HEIGHT));

    // Choose a colour depth.
    if(desktop_color_depth() != 0)
        set_color_depth(desktop_color_depth());
    else
        set_color_depth(16);

    // Suggest a good screen mode for OpenGL.
    allegro_gl_set(AGL_Z_DEPTH, get_color_depth());
    allegro_gl_set(AGL_COLOR_DEPTH, get_color_depth());
    allegro_gl_set(AGL_FULLSCREEN, use_fullscreen);
    allegro_gl_set(AGL_DOUBLEBUFFER, TRUE);
    allegro_gl_set(AGL_SUGGEST,
        AGL_FULLSCREEN | AGL_DOUBLEBUFFER | AGL_Z_DEPTH | AGL_COLOR_DEPTH);

    // Set the graphics mode.
    set_gfx_mode(GFX_OPENGL, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0);

    // Set the font
    if(exists(FONT_PATH))
    {
        common_gamefont = COMMON_load_texture(FONT_PATH);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    // Set some basic GL settings for speed
    glFogf(GL_FOG_MODE, GL_LINEAR);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_FOG_HINT, GL_FASTEST);

    // Set up the heartbeat
    common_time_for_logic = 0;
    LOCK_VARIABLE(common_time_for_logic);
    LOCK_FUNCTION(game_heartbeat);

    install_int_ex(game_heartbeat, BPS_TO_TIMER(TICKS_PER_SEC));

    // Set up the performance timer
    LOCK_VARIABLE(common_frames_per_sec);
    LOCK_VARIABLE(common_frames_this_sec);
    LOCK_FUNCTION(sample_framerate);

    install_int_ex(sample_framerate,BPS_TO_TIMER(1));

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glAlphaFunc(GL_GREATER, 0.5);
}


/**********************************************************************************************/
/*! \brief glflush() everything and copy the current contents of the double buffer to
 * the display.
 *
 * \note We also poll the music player and handle volume fades here. This is because  both have
 * to be called regularly many times a second, and redrawing has to happen many times a second,
 * so this seemed like the least worst place to put them.
 */
void COMMON_flip_buffer()
{
    // COMMON_fixup_mouse_coords(); // this breaks horribly with native desktop resolution

    if(common_duhplyr != NULL)
    {
        // check if a music change is pending
        if(common_should_fade_bgm)
        {
            // yes, fade out
            common_duhvol = common_duhvol * DUMB_FADE_MUL;
            al_duh_set_volume(common_duhplyr, common_duhvol);

            if(common_duhvol < DUMB_FADE_MIN)
                common_bgm_fade_done = TRUE;
        }
        al_poll_duh(common_duhplyr);
    }

    allegro_gl_flip();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    common_frames_this_sec++;
}

/**********************************************************************************************/
/*! \brief Show the number of frames drawn in the last second in the upper left corner. For
 * debugging, shouldn't be called in enduser-facing releases.
 */
void COMMON_show_fps()
{
    if(common_frames_per_sec < (TICKS_PER_SEC / 2))
        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    else if(common_frames_per_sec < (TICKS_PER_SEC / 1.5))
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
    else
        glColor4f(0.0f, 1.0f, 0.0f, 1.0f);

    COMMON_glprint(common_gamefont, 8, 8, 0, 16, 0, "FPS: %03d", common_frames_per_sec);
}

/**********************************************************************************************/
/*! \brief Load an image via Allegro and convert it into a ready-for-use texture.
 * \note Instead of crashing or asserting if it can't load an image, it'll instead
 * create a really-ugly bright green texture and print an error to the console.
 */
GLuint COMMON_load_texture(const char *filename)
{
    int format;
    GLuint ret_val;

    BITMAP *b;
    set_color_depth(24);
    set_color_conversion(COLORCONV_KEEP_ALPHA);

    if (exists(filename))
        b = load_bitmap(filename,NULL);
    else
    {
        DUH_WHERE_AM_I("missing image: %s", filename);
        b = create_bitmap_ex(24,1,1);
        putpixel(b,0,0,makecol24(0,255,0));
    }

    switch(bitmap_color_depth(b))
    {
        case 24:    format = GL_RGB;
                    break;

        case 32:    format = GL_RGBA;
                    break;

        case 16:    format = GL_RGB;
                    break;

        case 8:     format = GL_LUMINANCE;
                    break;

        default:   format = GL_RGB;
    }

    glBindTexture(GL_TEXTURE_2D, texture_name);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, b->w, b->h, 0, format, GL_UNSIGNED_BYTE, b->line[0]);

    destroy_bitmap(b);

    /// TODO: how come this doesn't use glGenTextures()?
    ret_val = texture_name;
    texture_name++;

    return ret_val;
}

/**********************************************************************************************/
/*!
 * \brief Switch to a perspective projection. Most 3D drawing should be done
 * in this mode.
 */
void COMMON_to_perspective(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glFrustum(-1.0f * common_aspect_ratio, 1.0f * common_aspect_ratio,
                -1.0f, 1.0f,
                2.0f, FAR_CLIP_DEPTH);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);

    common_persp_mode = TRUE;
}

/**********************************************************************************************/
/*!
 * \brief Switch to an orthographic projection and settings useful for drawing
 * dialog boxes, a HUD, a 2D-game, etc.
 *
 * \note the top left is the origin.
 */
void COMMON_to_ortho(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0f, (float)DISPLAY_HEIGHT * common_aspect_ratio,
            (float)DISPLAY_HEIGHT, 0.0f,
            -10.0f, 100.0f);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    common_persp_mode = FALSE;
}

/**********************************************************************************************/
/*!
 * \brief Stupid but effective replacement for gluLookAt() to ease porting to
 * OpenGL ES.
 */
void COMMON_look_at(GLfloat cam_x, GLfloat cam_y, GLfloat cam_z,
                GLfloat dest_x, GLfloat dest_y, GLfloat dest_z)
{
    float x_dist = dest_x - cam_x;
    float y_dist = dest_y - cam_y;
    float z_dist = dest_z - cam_z;

    // track the angles we set these to for later; we'll need them
    // to make sprites in perspective mode always face the camera.
    common_x_rot = atan2( y_dist, hypotf(x_dist, z_dist));
    common_y_rot = atan2(-x_dist, -z_dist);

    glRotatef(TO_DEGREES(-common_x_rot), 1, 0, 0);
    glRotatef(TO_DEGREES(-common_y_rot), 0, 1, 0);

    glTranslatef(-cam_x,-cam_y,-cam_z);
}

/**********************************************************************************************/
/*! \brief Turn off sound, music, timer, input and graphics subsystems and
 * cleans up. Should be called right before the end of main().
 */
void COMMON_shutdown(void)
{
    #ifdef  __DEBUG__
        fprintf(stderr,"\n");
    #endif

    clear_keybuf();
    remove_int(game_heartbeat);
    remove_int(sample_framerate);
    glDeleteTextures(1, &common_gamefont);
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}
