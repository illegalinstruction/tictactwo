#ifndef         SPLASH_SCREEN_H
    #define     SPLASH_SCREEN_H

    #include    "client-common.h"
    #include    "screenwipe.h"

    /*! \defgroup splash_pub_iface The public interface for the Splash Screen module.
     * \{ */
    void SPLASH_init(void);
    void SPLASH_tick(void);
    void SPLASH_draw(void);
    void SPLASH_load(void);
    void SPLASH_unload(void);
    /*! \} */

#endif
