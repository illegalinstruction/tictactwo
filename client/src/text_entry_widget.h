#ifndef         TEXT_ENTRY_WIDGET_H
    #define     TEXT_ENTRY_WIDGET_H

    #define     TEXT_ENTRY_WIDGET_STRING_LENGTH         MAX_NAME_LENGTH
    #define     TEXT_ENTRY_WIDGET_MODE_NORMAL           1
    #define     TEXT_ENTRY_WIDGET_MODE_NUMERIC          2
    #define     TEXT_ENTRY_WIDGET_MODE_NAME             3

/* not implemented yet;
    #define     TEXT_ENTRY_WIDGET_MODE_HIRAGANA         4
    #define     TEXT_ENTRY_WIDGET_MODE_KATAKANA         5
*/

    /*! \defgroup tewi_public_iface Public interface for the text-entry-widget.
     * \{ */
    void        TEWI_init(void);
    void        TEWI_tick(void);
    void        TEWI_draw(void);
    void        TEWI_enable_or_disable(BOOL should_be_enabled);
    BOOL        TEWI_check_active(void);
    void        TEWI_set_mode(uint8_t mode);
    int         TEWI_get_mode(void);
    char *      TEWI_get_string(void);
    void        TEWI_set_string(const char *in_str);
    void        TEWI_set_vpos(float vpos);

#endif
