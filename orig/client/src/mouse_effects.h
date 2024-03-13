/*******************************************************************************************
*
* Part of 'Jellybean Machine' - see common.h for copyright info.
*
*******************************************************************************************/

#ifndef      __MOUSE_EFFECTS_H
    #define  __MOUSE_EFFECTS_H

    /*! \defgroup mefx_public_iface The public interface of the mouse-effects module.
     * \{ */
    void MEFX_load(void);
    void MEFX_reset(void);
    void MEFX_spawn_shockring(void);
    void MEFX_tick(void);
    void MEFX_draw(void);
    void MEFX_unload(void);
    /*! \} */
#endif
