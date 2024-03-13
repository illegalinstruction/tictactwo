/*******************************************************************************
*
* Part of LD48 Entry: (working title) - see common.h for copyright info.
*
*******************************************************************************/

#include        "client-common.h"
#ifndef         __MS3D_MODEL_H
    #define     __MS3D_MODEL_H

    typedef struct
    {
        char id[10];                // magic number(always "MS3D000000")
        int  version;               // version of the spec model adheres to
    } MS3D_HEADER, *MS3D_HEADER_PTR;

    typedef struct
    {
        unsigned char   flags;
        unsigned char   ref_count;
        char            bone_id;    //The bone ID (used for skeletal animation)
        float           vertex[3];
    } MS3D_VERTEX, *MS3D_VERTEX_PTR;

    typedef struct
    {
        unsigned short  flags;
        unsigned short  vertexIndices[3];
        float               vertexNormals[3][3];
        float               u[3];
        float               v[3];
        unsigned char   smoothing_group;
        unsigned char   group_index;
    } MS3D_TRIANGLE, *MS3D_TRIANGLE_PTR;

    typedef struct
    {
        unsigned char       flags;
        char                name[32];
        unsigned short  num_triangles;
        unsigned short* triangle_indices;
        char                material_index;
    } MS3D_GROUP, *MS3D_GROUP_PTR;

    typedef struct
    {
        // the bits and bobs that make up the Milshape file.
        unsigned short  num_vertices;
        MS3D_VERTEX*        vertices;
        unsigned short  num_triangles;
        MS3D_TRIANGLE*  triangles;
        unsigned short  num_groups;
        MS3D_GROUP*     groups;
        unsigned short  num_materials;

        // the bits we're actually going to upload and draw.
        VERTEX*         processed_verts;
        NORMAL*         processed_norms;
        TEX_COORD*      processed_uvs;

    } MS3D_MODEL;

    /**************************************************************************/

    MS3D_MODEL *    MS3D_load(const char *filename);
    void                MS3D_unload(MS3D_MODEL * model);
    void                MS3D_preprocess_vbos(MS3D_MODEL * model);
    void                MS3D_draw(const MS3D_MODEL * model, GLuint texture,
                                    GLfloat x, GLfloat y, GLfloat z, GLfloat y_rot);
#endif
