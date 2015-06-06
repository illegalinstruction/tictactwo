/*******************************************************************************
*
* Part of LD48 Entry: (working title) - see common.h for copyright info.
*
*******************************************************************************/

#include "md2_model.h"
#include "md2_normals.h"

/******************************************************************************/
/*!
 * Load a Quake II model from disk and fill out an MD2_MODEL data structure
 * with it.
 *
 * Returns null if it couldn't find or open the specified file.
 */
MD2_MODEL * MD2_load(const char *filename)
{
    MD2_HEADER_STRUCT   header;
    FILE*                   fin;
    int                     index;
    int                     element_index;
    int                     dest;
    short                   tmp;

    MD2_MODEL*              ret;

    if(!exists(filename))
    {
        return NULL;
    }

    fin = fopen(filename,"rb");
    fread(&header,sizeof(MD2_HEADER_STRUCT),1,fin);

    ret = (MD2_MODEL*)malloc(sizeof(MD2_MODEL));

    if(ret == NULL)
    {
        fclose(fin);
        return NULL;
    }

    // get how much geometry we need to store
    ret->num_frames = header.num_frames;
    ret->num_triangles = header.num_triangles;
    ret->num_vertices = header.num_vertices;
    ret->num_texcoords = header.num_texture_coords;

    // allocate enough space to hold the geometry
    ret->anim_frames = (MD2_ANIM_FRAME_STRUCT *)malloc(sizeof(MD2_ANIM_FRAME_STRUCT) * ret->num_frames);

    for(index = 0; index < header.num_frames; index++)
    {
        ret->anim_frames[index].vertices = (MD2_VERTEX_STRUCT *)malloc(sizeof(MD2_VERTEX_STRUCT) * ret->num_vertices);
    }

    ret->triangles  = (MD2_TRIANGLE *)malloc(sizeof(MD2_TRIANGLE) * ret->num_triangles);

    ret->vert_array = (DRAWABLE_VERT *)malloc(sizeof(DRAWABLE_VERT) * ret->num_triangles * 3);
    ret->norm_array = (DRAWABLE_VERT *)malloc(sizeof(DRAWABLE_VERT) * ret->num_triangles * 3);
    ret->uv_array   = (DRAWABLE_UV *)malloc(sizeof(DRAWABLE_UV) * ret->num_triangles * 3);

    fseek(fin,header.offset_texture_coords,SEEK_SET);

    ret->tex_coords = (MD2_TEXCOORD*)malloc(sizeof(MD2_TEXCOORD) * ret->num_texcoords);

    for(index = 0; index <  header.num_texture_coords; index++)
    {
        fread(&tmp,sizeof(short),1,fin);
        ret->tex_coords[index].u = tmp / (float)header.skin_width;

        fread(&tmp,sizeof(short),1,fin);
        ret->tex_coords[index].v = tmp / (float)header.skin_height;
    }

    // read vertex and uv coord indices.
    fseek(fin,header.offset_triangles,SEEK_SET);

    for(element_index = 0; element_index < header.num_triangles; element_index++)
    {
        fread(&ret->triangles[element_index].vertex_indices[0],sizeof(short),1,fin);
        fread(&ret->triangles[element_index].vertex_indices[1],sizeof(short),1,fin);
        fread(&ret->triangles[element_index].vertex_indices[2],sizeof(short),1,fin);

        fread(&ret->triangles[element_index].texture_indices[0],sizeof(short),1,fin);
        fread(&ret->triangles[element_index].texture_indices[1],sizeof(short),1,fin);
        fread(&ret->triangles[element_index].texture_indices[2],sizeof(short),1,fin);
    }

    // read actual frame details and encoded verts
    fseek(fin,header.offset_frames,SEEK_SET);

    // it isn't clear why, but the coordinates are stored x, z, y rather
    // than x, y, z...? endianness confusion on my part, perhaps
    for(index = 0; index < ret->num_frames; index++)
    {
        element_index   = 0;

        fread(&ret->anim_frames[index].scale_x,sizeof(float),1,fin);
        fread(&ret->anim_frames[index].scale_y,sizeof(float),1,fin);
        fread(&ret->anim_frames[index].scale_z,sizeof(float),1,fin);

        fread(&ret->anim_frames[index].translate_x,sizeof(float),1,fin);
        fread(&ret->anim_frames[index].translate_z,sizeof(float),1,fin);
        fread(&ret->anim_frames[index].translate_y,sizeof(float),1,fin);

        fseek(fin, 16, SEEK_CUR);

        do
        {
            fread(&ret->anim_frames[index].vertices[element_index].unscaled_x, sizeof(char), 1, fin);
            fread(&ret->anim_frames[index].vertices[element_index].unscaled_z, sizeof(char), 1, fin);
            fread(&ret->anim_frames[index].vertices[element_index].unscaled_y, sizeof(char), 1, fin);
            fread(&ret->anim_frames[index].vertices[element_index].normal_list_index, sizeof(char), 1, fin);
            element_index++;
        }
        while(element_index < header.num_vertices);
    }

    fclose(fin);

    MD2_preprocess_uvs(ret);

    return ret;
}

/******************************************************************************/
/*!
 * Process this model's position and surface normal data and prepare it for
 * drawing.
 */
void MD2_process_geometry(MD2_MODEL *model, int curr_frame, int next_frame, float tween)
{
    int tri_index;

    float one_minus_tween = 1.0f - tween;

    float scaled_x;
    float scaled_y;
    float scaled_z;

    // check if we're processing a model that we don't need to be...
    if(model->triangles == NULL) return;

    for(tri_index = 0; tri_index < model->num_triangles; tri_index++)
    {
        //========= first point ==========
        //--- vertex ----
        scaled_x =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[0]].unscaled_x *
                            model->anim_frames[curr_frame].scale_x + model->anim_frames[curr_frame].translate_x) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[0]].unscaled_x *
                            model->anim_frames[next_frame].scale_x + model->anim_frames[next_frame].translate_x) * tween);

        scaled_y =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[0]].unscaled_y *
                            model->anim_frames[curr_frame].scale_y + model->anim_frames[curr_frame].translate_y) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[0]].unscaled_y *
                            model->anim_frames[next_frame].scale_y + model->anim_frames[next_frame].translate_y) * tween);

        scaled_z =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[0]].unscaled_z *
                            model->anim_frames[curr_frame].scale_z + model->anim_frames[curr_frame].translate_z) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[0]].unscaled_z *
                            model->anim_frames[next_frame].scale_z + model->anim_frames[next_frame].translate_z) * tween);

        model->vert_array[tri_index * 3 + 0].x = scaled_x;
        model->vert_array[tri_index * 3 + 0].y = scaled_y;
        model->vert_array[tri_index * 3 + 0].z = scaled_z;

        //--- normal ----
        scaled_x = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[0]].normal_list_index][ 0] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[0]].normal_list_index][ 0] * tween);
        scaled_y = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[0]].normal_list_index][ 1] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[0]].normal_list_index][ 1] * tween);
        scaled_z = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[0]].normal_list_index][ 2] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[0]].normal_list_index][ 2] * tween);

        model->norm_array[tri_index * 3 + 0].x = scaled_x;
        model->norm_array[tri_index * 3 + 0].z = scaled_y;
        model->norm_array[tri_index * 3 + 0].y = scaled_z;

        //========= second point ==========
        //--- vertex ----
        scaled_x =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[1]].unscaled_x *
                            model->anim_frames[curr_frame].scale_x + model->anim_frames[curr_frame].translate_x) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[1]].unscaled_x *
                            model->anim_frames[next_frame].scale_x + model->anim_frames[next_frame].translate_x) * tween);

        scaled_y =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[1]].unscaled_y *
                            model->anim_frames[curr_frame].scale_y + model->anim_frames[curr_frame].translate_y) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[1]].unscaled_y *
                            model->anim_frames[next_frame].scale_y + model->anim_frames[next_frame].translate_y) * tween);

        scaled_z =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[1]].unscaled_z *
                            model->anim_frames[curr_frame].scale_z + model->anim_frames[curr_frame].translate_z) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[1]].unscaled_z *
                            model->anim_frames[next_frame].scale_z + model->anim_frames[next_frame].translate_z) * tween);

        model->vert_array[tri_index * 3 + 1].x = scaled_x;
        model->vert_array[tri_index * 3 + 1].y = scaled_y;
        model->vert_array[tri_index * 3 + 1].z = scaled_z;

        //--- normal ----
        scaled_x = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[1]].normal_list_index][ 0] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[1]].normal_list_index][ 0] * tween);
        scaled_y = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[1]].normal_list_index][ 1] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[1]].normal_list_index][ 1] * tween);
        scaled_z = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[1]].normal_list_index][ 2] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[1]].normal_list_index][ 2] * tween);

        model->norm_array[tri_index * 3 + 1].x = scaled_x;
        model->norm_array[tri_index * 3 + 1].z = scaled_y;
        model->norm_array[tri_index * 3 + 1].y = scaled_z;

        //========= third point ==========
        //--- vertex ----
        scaled_x =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[2]].unscaled_x *
                            model->anim_frames[curr_frame].scale_x + model->anim_frames[curr_frame].translate_x) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[2]].unscaled_x *
                            model->anim_frames[next_frame].scale_x + model->anim_frames[next_frame].translate_x) * tween);

        scaled_y =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[2]].unscaled_y *
                            model->anim_frames[curr_frame].scale_y + model->anim_frames[curr_frame].translate_y) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[2]].unscaled_y *
                            model->anim_frames[next_frame].scale_y + model->anim_frames[next_frame].translate_y) * tween);

        scaled_z =  ((  (float)model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[2]].unscaled_z *
                            model->anim_frames[curr_frame].scale_z + model->anim_frames[curr_frame].translate_z) * one_minus_tween) +
                        ((  (float)model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[2]].unscaled_z *
                            model->anim_frames[next_frame].scale_z + model->anim_frames[next_frame].translate_z) * tween);

        model->vert_array[tri_index * 3 + 2].x = scaled_x;
        model->vert_array[tri_index * 3 + 2].y = scaled_y;
        model->vert_array[tri_index * 3 + 2].z = scaled_z;

        //--- normal ----
        scaled_x = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[2]].normal_list_index][ 0] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[2]].normal_list_index][ 0] * tween);
        scaled_y = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[2]].normal_list_index][ 1] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[2]].normal_list_index][ 1] * tween);
        scaled_z = (normal_list[ model->anim_frames[curr_frame].vertices[model->triangles[tri_index].vertex_indices[2]].normal_list_index][ 2] * one_minus_tween) +
            (normal_list[ model->anim_frames[next_frame].vertices[model->triangles[tri_index].vertex_indices[2]].normal_list_index][ 2] * tween);

        model->norm_array[tri_index * 3 + 2].x = scaled_x;
        model->norm_array[tri_index * 3 + 2].z = scaled_y;
        model->norm_array[tri_index * 3 + 2].y = scaled_z;
    }
}

/******************************************************************************/
/*!
 * Preprocess the UV coordinates, since these don't change between frames
 * in a Quake model.
 */
void MD2_preprocess_uvs(MD2_MODEL * model)
{
    int  tri_index;

    if(model == NULL) return;

    for(tri_index = 0; tri_index < model->num_triangles; tri_index++)
    {
        //========= first point ==========
        model->uv_array[tri_index * 3 + 0].u = model->tex_coords[model->triangles[tri_index].texture_indices[0]].u;
        model->uv_array[tri_index * 3 + 0].v = model->tex_coords[model->triangles[tri_index].texture_indices[0]].v;

        //========= second point ==========
        model->uv_array[tri_index * 3 + 1].u = model->tex_coords[model->triangles[tri_index].texture_indices[1]].u;
        model->uv_array[tri_index * 3 + 1].v = model->tex_coords[model->triangles[tri_index].texture_indices[1]].v;

        //========= third point ==========
        model->uv_array[tri_index * 3 + 2].u = model->tex_coords[model->triangles[tri_index].texture_indices[2]].u;
        model->uv_array[tri_index * 3 + 2].v = model->tex_coords[model->triangles[tri_index].texture_indices[2]].v;
    }
}

/******************************************************************************/
/*!
 * Draw the specified model with the specified texture at the specified
 * coordinates, with the specified animation frames and tweening.
 */
void MD2_draw(MD2_MODEL * model, GLuint tex, GLfloat x, GLfloat y, GLfloat z,  GLfloat y_rot, int curr_frame, int next_frame, float tween)
{
    float   scaled_x0, scaled_y0, scaled_z0;
    float   scaled_x1, scaled_y1, scaled_z1;
    float   scaled_x, scaled_y, scaled_z;

    int     tri_index;

    if(model == NULL) return;

    glPushMatrix();

    glTranslatef(x,y,z);
    glRotatef(y_rot,0,1,0);
    glBindTexture(GL_TEXTURE_2D, tex);

    MD2_process_geometry(model, curr_frame, next_frame, tween);

    glVertexPointer(3,GL_FLOAT,0,model->vert_array);
    glTexCoordPointer(2,GL_FLOAT,0,model->uv_array);
    glNormalPointer(GL_FLOAT,0,model->norm_array);

    glDrawArrays(GL_TRIANGLES,0,(model->num_triangles) * 3);

    glPopMatrix();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/******************************************************************************/
/*!
 * Discard the specified model and free up memory.
 */
void MD2_unload(MD2_MODEL *ret)
{
    int index;

    if(ret == NULL)
    {
        // nothing to do here
        return;
    }

    if(ret->tex_coords) free(ret->tex_coords);
    if(ret->vert_array) free(ret->vert_array);
    if(ret->norm_array) free(ret->norm_array);
    if(ret->uv_array)   free(ret->uv_array);
    if(ret->triangles)  free(ret->triangles);

    for(index = 0; index < ret->num_frames; index++)
    {
        free(ret->anim_frames[index].vertices);
    }

    free(ret->anim_frames);

    free(ret);
}
