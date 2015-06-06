#ifndef         YOU_WIN_H
    #define     YOU_WIN_H

    #include    "client-common.h"
    #include    "screenwipe.h"

    /*! \defgroup stats_screen_iface The public interface to the end-of-round-screen's module.
     * \{ */
    void YOUWIN_init(void);
    void YOUWIN_tick(void);
    void YOUWIN_draw(void);
    void YOUWIN_load(void);
    void YOUWIN_unload(void);
    /*! \} */
#endif
