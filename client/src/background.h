/*! \file background.h
 * \brief Contains functionality for loading, animating and displaying background artwork.
 * \note This is kind of designed to be loaded ONCE, at the start; it can safely go behind
 * any gamestate.
 */

#ifndef         BACKGROUND_H
    #define     BACKGROUND_H

    #include    "tictactwo-common.h"

    void        BKGRND_load(void);
    void        BKGRND_tick(void);
    void        BKGRND_draw(void);

#endif
