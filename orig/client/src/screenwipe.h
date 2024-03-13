/*! \file screenwipe.h
 * \brief Contains functions for working with the screen transition animation.
 */
#ifndef         SCREENWIPE_H
    #define     SCREENWIPE_H

    #include    "client-common.h"

    void        SCRNWIPE_init(void);
    void        SCRNWIPE_start(BOOL direction);
    void        SCRNWIPE_tick(void);
    void        SCRNWIPE_draw(void);
    BOOL        SCRNWIPE_check_active(void);
    BOOL        SCRNWIPE_check_direction(void);

#endif
