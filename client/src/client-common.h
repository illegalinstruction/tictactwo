/*! \file client-common.h
 *  \brief Functionality needed to implement the client (such as
 *   graphics, sound, timing, etc.
 *  \note Astute readers will notice that it reinvents a lot of stuff
 *   allegrogl already knows how to do; this is deliberate.  allegrogl
 *   can't be used with OpenGL ES, so the reinvented-square-wheel stuff
 *   takes its place.  This is intended to ease porting to OpenPandora
 *   or RPi later.
 */
#include        <stdio.h>
#include        <stdint.h>
#include        <stdlib.h>
#include        <math.h>
#include        <allegro.h>
#include        <aldumb.h>
#include        <alleggl.h>
#include        "tictactwo-common.h"
#define         glFrustumf      glFrustum
#define         glOrthof        glOrtho

#ifndef         CLIENT_COMMON_H
    #define     CLIENT_COMMON_H

    #define     FONT_PATH     "./gfx/font/gamefont.tga"

    #define     TICKS_PER_SEC   50

    /*! \defgroup display_size_defines
     *  \brief Defines that manage the size and resolution of
     *   the display and viewing volume.
     *  \{
     */
    #define     DISPLAY_WIDTH   800
    #define     DISPLAY_HEIGHT  600
    #define     FAR_CLIP_DEPTH  4096.0f
    /*! \} */

    /*! \defgroup trig_helpers
     *  \brief Helper macros for converting to and from degrees
     *   or radians.
     *  \{
     */
    #define     TO_RADIANS(theta)   (((theta * M_PI) / 180.0))
    #define     TO_DEGREES(theta)   ((theta * (180.0 / M_PI)))
    /*! \} */

    /*! \defgroup dumb_macros "DUMB initialization-related stuff"
     *  \brief Defines that control the initialisation and use of the
     *    music player library DUMB.
     * \{
     */
    #define     DUMB_BUFSIZE    8192
    #define     DUMB_MIXRATE    44100
    #define     DUMB_FADE_MUL   0.95f
    #define     DUMB_FADE_MIN   0.01f
    #define     DUMB_VOL        0.96f
    /*! \} */

    /*! \defgroup bgm_macros "Background Music IDs"
     *  \brief Defines that give symbolic names to the BGM ident numbers.
     * \{
     */
    #define     BGM_ID_MAINMENU_LOBBY       99
    #define     BGM_ID_INGAME               1
    #define     BGM_ID_END_OF_ROUND         98
    /*! \} */

    #define     INCOMING_CHAT_COLOR         111, 255, 64
    #define     INCOMING_CHAT_SIZE          19
    #define     INCOMING_CHAT_KERN          -4
    #define     INCOMING_CHAT_X             80
    #define     INCOMING_CHAT_Y             512
    #define     INCOMING_CHAT_TIMEOUT       (TICKS_PER_SEC * 4.5f)

    /*! \brief A structure that holds what to do for any of the states
     * the application can be in.  A state is a self-contained area
     * of the application, like the main menu, the high score screen,
     * etc.
     */
    typedef struct
    {
        void    (*init)();
        void    (*load)();
        void    (*unload)();
        void    (*tick)();
        void    (*draw)();
    } GAMESTATE;

    /*! \brief Represents a point in 3D space. */
    typedef struct
    {
        GLfloat x;
        GLfloat y;
        GLfloat z;
    }
    VERTEX;

    /*! \brief Represents a point in 2D texturespace. */
    typedef struct
    {
        GLfloat u;
        GLfloat v;
    }
    TEX_COORD;

    /*! \brief Represents a vector that tells which way a triangle is 'pointing'. */
    typedef struct
    {
        GLfloat x;
        GLfloat y;
        GLfloat z;
    }
    NORMAL;

    /*! \brief Represents a vector that describes a colour. */
    typedef struct
    {
        GLfloat r;
        GLfloat g;
        GLfloat b;
        GLfloat a;
    } COLOR;


    /*! \brief A little sugar to make it clearer when a new texture's being bound, etc. */
    typedef GLuint         TEXTURE_HANDLE;

    /*! \brief A value that represents the number of virtual pixels across the display. */
    extern float           common_effective_display_width;
    /*! \brief A flag that tells client code whether the background music is completely faded out. */
    extern int             common_bgm_fade_done;

    /*! \defgroup gamestate_flags
     * \brief Flags to help track what gamestate we're in, whether it's done or not, etc.
     * \{ */
    extern int             common_curr_state;
    extern BOOL             common_curr_state_done;
    extern int             common_next_state;
    extern int             common_next_state_param;
    /*! \} */

    extern volatile int     common_time_for_logic;
    extern TEXTURE_HANDLE   common_gamefont;
    extern char             common_user_home_path[];
    extern BOOL             common_persp_mode;

    void    COMMON_setup(int use_fullscreen);
    GLuint  COMMON_load_texture(const char *filename);
    void    COMMON_look_at(GLfloat cam_x, GLfloat cam_y, GLfloat cam_z, GLfloat dest_x, GLfloat dest_y, GLfloat dest_z);
    void    COMMON_shutdown();
    void    COMMON_glprint(TEXTURE_HANDLE font, GLfloat x, GLfloat y, GLfloat z, GLfloat size, GLfloat spacing, const char *str, ...);
    void    COMMON_flip_buffer();
    void    COMMON_show_fps();
    void    COMMON_fade_bgm(void);
    void    COMMON_set_bgm(int track);
    int     COMMON_get_bgm(void);
    void    COMMON_to_ortho(void);
    void    COMMON_to_perspective(void);
    void    COMMON_draw_sprite(TEXTURE_HANDLE sprite, float x, float  y, float  z, int w, int h);
    BOOL    COMMON_connect(uint32_t remote_address, uint16_t remote_port);
    BOOL    COMMON_send(void *data, uint16_t length);
    BOOL    COMMON_recv(void *data, uint16_t length);
    void    COMMON_disconnect(void);
    char    *COMMON_load_file_to_char_array(const char *filename);


#endif
