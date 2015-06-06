/*******************************************************************************
*
* Part of LD48 Entry: (working title) - see common.h for copyright info.
*
*******************************************************************************/

#include "ms3d_model.h"

/******************************************************************************/
/*!
 * Reads the contents of an .ms3d file and prepares the model for use
 * with OpenGL ES. It does NOT respect different materials or groups;
 * this is by design, since it's really only being used to draw the
 * landscape.
 *
 * At least some of this code came from an old Trent Pollack tutorial,
 * but I changed a lot of things. The original URL has been lost, but
 * http://nehe.gamedev.net/tutorial/model_loading/16004/ contains the
 * same info.
 */
MS3D_MODEL * MS3D_load(const char *filename)
{
    FILE*               file;
    MS3D_HEADER         header;
    MS3D_MODEL *        model = NULL;
    int                 index;

    if(exists(filename))
    {
        file = fopen(filename, "rb");

        if(file == NULL)
        {
            return NULL;
        }

        // read the header data in.
        fread(&header.id, sizeof(char), 10, file);
        fread(&header.version, 1, sizeof(int), file);

        // is this really a milkshape model?
        if(strncmp(header.id, "MS3D000000", 10)!=0)
        {
            // nope.
            DUH_WHERE_AM_I("invalid file.");
            return NULL;
        }

        model = (MS3D_MODEL *)malloc(sizeof(MS3D_MODEL));

        if(model == NULL)
        {
            // if this is getting executed, try closing
            // firefox and/or reboot your pandora, please
            DUH_WHERE_AM_I("malloc failed.");
            return NULL;
        }

        // read the vertex data in.
        fread(&model->num_vertices, sizeof(unsigned short), 1, file);
        model->vertices= (MS3D_VERTEX_PTR)malloc(sizeof(MS3D_VERTEX) * model->num_vertices);

        for(index=0; index < model->num_vertices; index++)
        {
            fread(&model->vertices[index].flags,    sizeof(unsigned char),  1, file);
            fread( model->vertices[index].vertex,   sizeof(float),          3, file);
            fread(&model->vertices[index].bone_id,  sizeof(char),           1, file);
            fread(&model->vertices[index].ref_count, sizeof(unsigned char),  1, file);
        }

        // read the triangle indices in.
        fread(&model->num_triangles, sizeof(unsigned short), 1, file);
        model->triangles= (MS3D_TRIANGLE_PTR)malloc(sizeof(MS3D_TRIANGLE) * model->num_triangles);

        for(index = 0; index < model->num_triangles; index++)
        {
            fread(&model->triangles[index].flags,            sizeof(unsigned short),    1, file);
            fread( model->triangles[index].vertexIndices,    sizeof(unsigned short), 3, file);
            fread( model->triangles[index].vertexNormals[0], sizeof(float),         3, file);
            fread( model->triangles[index].vertexNormals[1], sizeof(float),         3, file);
            fread( model->triangles[index].vertexNormals[2], sizeof(float),         3, file);
            fread( model->triangles[index].u,                sizeof(float),         3, file);
            fread( model->triangles[index].v,                sizeof(float),         3, file);
            fread(&model->triangles[index].smoothing_group,  sizeof(unsigned char),  1, file);
            fread(&model->triangles[index].group_index,      sizeof(unsigned char),  1, file);
        }

        // read the group data in
        fread(&model->num_groups, sizeof(unsigned short), 1, file);
        model->groups = (MS3D_GROUP_PTR)malloc(sizeof(MS3D_GROUP) * model->num_groups);

        for(index = 0; index < model->num_groups; index++)
        {
            fread(&model->groups[index].flags,           sizeof(unsigned char),  1, file);
            fread( model->groups[index].name,            sizeof(char),              32, file);
            fread(&model->groups[index].num_triangles,   sizeof(unsigned short), 1, file);

            model->groups[index].triangle_indices   =  (unsigned short *)malloc(sizeof(unsigned short) * model->groups[index].num_triangles);

            fread( model->groups[index].triangle_indices,   sizeof(unsigned short), model->groups[index].num_triangles,file);
            fread(&model->groups[index].material_index,     sizeof(char), 1, file);
        }

        // make sure our vbo stuff doesn't point anywhere stupid.
        model->processed_verts  = NULL;
        model->processed_norms  = NULL;
        model->processed_uvs    = NULL;

        fclose(file);

        // prepare it to be drawn.
        MS3D_preprocess_vbos(model);
        return model;
    }

    DUH_WHERE_AM_I("file didn't exist.");
    return NULL;
}

/******************************************************************************/
/*!
 * Releases memory used by an ms3d model.
 */
void MS3D_unload(MS3D_MODEL * model)
{
    if(model == NULL) return;

    if(model->processed_norms)
        free(model->processed_norms);

    if(model->processed_verts)
        free(model->processed_verts);

    if(model->processed_uvs)
        free(model->processed_uvs);

    if(model->vertices)
        free(model->vertices);

    if(model->groups)
        free(model->groups);

    if(model->triangles)
        free(model->triangles);

    if(model)
    {
        free(model);
        model = NULL;
    }

    return;
}

/******************************************************************************/
/*!
 * Process a milkshape model into vertex buffer objects.
 *
 * N.B.: This gleefully and completely disregards groups.  We'll only be using
 * this format for the land, for which all models share one texture,
 * so group isn't needed.
 */
void MS3D_preprocess_vbos(MS3D_MODEL * model)
{
    int grp_index;
    int tri_index;
    int vert_index;

    if(model == NULL) return;

    model->processed_verts  = (VERTEX *)malloc(model->num_triangles * 3 * sizeof(VERTEX));
    model->processed_norms  = (NORMAL *)malloc(model->num_triangles * 3 * sizeof(NORMAL));
    model->processed_uvs    = (TEX_COORD *)malloc(model->num_triangles * 3 * sizeof(TEX_COORD));

    // process all geometry and put it into ES-friendly arrays
    for(grp_index = 0; grp_index < model->num_groups; grp_index++)
    {
        for(tri_index = 0; tri_index < model->groups[grp_index].num_triangles; tri_index++)
        {
            // ugh, horrid indirection abuse.  why did I do it this way before?
            const MS3D_TRIANGLE* tri = &model->triangles[ model->groups[grp_index].triangle_indices[tri_index] ];

            // first vertex.
            vert_index  = tri->vertexIndices[0];

            model->processed_norms[(tri_index * 3) + 0].x = tri->vertexNormals[0][0];
            model->processed_norms[(tri_index * 3) + 0].y = tri->vertexNormals[0][1];
            model->processed_norms[(tri_index * 3) + 0].z = tri->vertexNormals[0][2];

            model->processed_uvs[(tri_index * 3) + 0].u = tri->u[0];
            model->processed_uvs[(tri_index * 3) + 0].v = tri->v[0];

            model->processed_verts[(tri_index * 3) + 0].x = model->vertices[vert_index].vertex[0];
            model->processed_verts[(tri_index * 3) + 0].y = model->vertices[vert_index].vertex[1];
            model->processed_verts[(tri_index * 3) + 0].z = model->vertices[vert_index].vertex[2];

            // second vertex.
            vert_index  = tri->vertexIndices[1];

            model->processed_norms[(tri_index * 3) + 1].x = tri->vertexNormals[1][0];
            model->processed_norms[(tri_index * 3) + 1].y = tri->vertexNormals[1][1];
            model->processed_norms[(tri_index * 3) + 1].z = tri->vertexNormals[1][2];

            model->processed_uvs[(tri_index * 3) + 1].u = tri->u[1];
            model->processed_uvs[(tri_index * 3) + 1].v = tri->v[1];

            model->processed_verts[(tri_index * 3) + 1].x = model->vertices[vert_index].vertex[0];
            model->processed_verts[(tri_index * 3) + 1].y = model->vertices[vert_index].vertex[1];
            model->processed_verts[(tri_index * 3) + 1].z = model->vertices[vert_index].vertex[2];

            // third vertex.
            vert_index  = tri->vertexIndices[2];

            model->processed_norms[(tri_index * 3) + 2].x = tri->vertexNormals[2][0];
            model->processed_norms[(tri_index * 3) + 2].y = tri->vertexNormals[2][1];
            model->processed_norms[(tri_index * 3) + 2].z = tri->vertexNormals[2][2];

            model->processed_uvs[(tri_index * 3) + 2].u = tri->u[2];
            model->processed_uvs[(tri_index * 3) + 2].v = tri->v[2];

            model->processed_verts[(tri_index * 3) + 2].x = model->vertices[vert_index].vertex[0];
            model->processed_verts[(tri_index * 3) + 2].y = model->vertices[vert_index].vertex[1];
            model->processed_verts[(tri_index * 3) + 2].z = model->vertices[vert_index].vertex[2];
        }
    }

    free(model->vertices);
    model->vertices = NULL;

    free(model->groups);
    model->groups = NULL;

    free(model->triangles);
    model->triangles = NULL;
}

/******************************************************************************/
/*!
 * Paint a preprocessed milkshape model to the screen.
 */
void MS3D_draw(const MS3D_MODEL * model, GLuint texture, GLfloat x, GLfloat y, GLfloat z, GLfloat y_rot)
{
    if(model == NULL) return;

    glBindTexture(GL_TEXTURE_2D, texture);

    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(y_rot,0,1,0);

    glVertexPointer(3,GL_FLOAT,0,model->processed_verts);
    glNormalPointer(GL_FLOAT,0,model->processed_norms);
    glTexCoordPointer(2,GL_FLOAT,0,model->processed_uvs);

    glDrawArrays(GL_TRIANGLES,0,model->num_triangles * 3);

    glPopMatrix();
}
