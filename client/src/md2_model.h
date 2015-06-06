/*******************************************************************************
*
* Part of LD48 Entry: (working title) - see common.h for copyright info.
*
*******************************************************************************/

#ifndef         __MD2_MODEL_H
    #define     __MD2_MODEL_H

    #include    "client-common.h"

    #ifdef      __OPENPANDORA__
        #include <GLES/gl.h>
        #include <GLES/egl.h>
    #else
        #include <alleggl.h>
    #endif


    typedef struct
    {
        unsigned int  ident;
        unsigned int  version;
        unsigned int  skin_width;       /* ignored */
        unsigned int  skin_height;      /* ignored */
        unsigned int  frame_size;
        unsigned int  num_skins;        /* ignored */
        unsigned int  num_vertices;
        unsigned int  num_texture_coords;
        unsigned int  num_triangles;
        unsigned int  num_gl_commands;
        unsigned int  num_frames;
        unsigned int  offset_skins;     /* ignored */
        unsigned int  offset_texture_coords;
        unsigned int  offset_triangles;
        unsigned int  offset_frames;
        unsigned int  offset_gl_commands;
        unsigned int  offset_end;
    } MD2_HEADER_STRUCT;

    /************************************************************/

    typedef struct
    {
        GLfloat u;
        GLfloat v;
    }
    MD2_TEXCOORD;

    /************************************************************/

    typedef struct
    {
        unsigned char unscaled_x;
        unsigned char unscaled_y;
        unsigned char unscaled_z;
        unsigned char normal_list_index; /* using the id surface normals */
    } MD2_VERTEX_STRUCT;

    /*****************************************************************/

    typedef struct
    {
        float x;
        float y;
        float z;
    } DRAWABLE_VERT;

    typedef struct
    {
        float u;
        float v;
    } DRAWABLE_UV;


    /*****************************************************************/

    typedef struct
    {
        float           tex_coord_x;
        float           tex_coord_y;
        unsigned int    vertex_list_index;
    } MD2_TRISTRIP_TRIFAN_ELEMENT;

    /*****************************************************************/

    typedef struct
    {
        short      vertex_indices[3];
        short      texture_indices[3];
    } MD2_TRIANGLE;

    /*****************************************************************/

    typedef struct
    {
        float                   scale_x;
        float                   scale_y;
        float                   scale_z;
        float                   translate_x;
        float                   translate_y;
        float                   translate_z;
        MD2_VERTEX_STRUCT*  vertices;
    } MD2_ANIM_FRAME_STRUCT;

    /*****************************************************************/

    typedef struct
    {
        MD2_ANIM_FRAME_STRUCT*      anim_frames;
        MD2_TRIANGLE*               triangles;
        MD2_TEXCOORD*               tex_coords;
        DRAWABLE_VERT*              vert_array;
        DRAWABLE_VERT*              norm_array;
        DRAWABLE_UV*                uv_array;
        unsigned int                num_frames;
        unsigned int                num_triangles;
        unsigned int                num_texcoords;
        unsigned int                num_drawable;
        unsigned int                num_vertices;
        GLuint                      vbo_ids[3];
    } MD2_MODEL;

    /*****************************************************************/

    MD2_MODEL*  MD2_load(const char* filename);
    void            MD2_preprocess_uvs(MD2_MODEL* model);
    void            MD2_draw(MD2_MODEL* model, GLuint tex,
                            GLfloat x, GLfloat y, GLfloat z,
                            GLfloat y_rot,
                            int curr_frame, int next_frame, float tween);
    void            MD2_unload(MD2_MODEL* model);
#endif
