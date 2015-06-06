/*! \file player_db.c
 * \todo Gut this entire thing and replace it with SQLite.
 * \note Removed the stupid 'middle' pointer, since it was a cause of no end of woe, and
 *  realistically, how many players are we ever going to get at a time anyway?  If this
 *  somehow becomes a problem, we'll just jump ship for SQLite.
 */

#include    <stdio.h>
#include    <malloc.h>
#include    "player_db.h"

/*! \brief The path to the on-disk backing file for the player list. */
#define     PLAYERDB_FILE_PATH "./.tictac2_playerlist.db"
/*! \brief The path to a backup so minimal stats are lost if something goes wrong. */
#define     PLAYERDB_BKUP_PATH "./.tictac2_playerlist.db.bak"

/*! \defgroup plyrdb_module_private
 * \brief Private functions and data internal to the player DB module.
 * \{
 */

/*! \brief Tracks whether we've loaded the db or not - this needs to happen before we search or append... */
static  BOOL    plyrdb_module_inited    = FALSE;

/*! \brief The player list.*/
static PLAYER_STRUCT *plyrdb_playerdb   = NULL;

/*! \brief Where list insertion actually happens. */
static void PLYRDB_insert_helper(PLAYER_STRUCT *tmp);

/*! \brief Frees up the memory used by the player list; designed to be called ONCE, on exit. */
static void PLYRDB_cleanup(void);

/*! \} */


/****************************************************************************************************************/
/*! \brief Finds the player with the specified name and returns them, or NULL if they weren't in the list.
 * \param name The name of the player to look for.
 */
PLAYER_STRUCT *PLYRDB_find_by_name(const char *name)
{
    PLAYER_STRUCT *list_walk = NULL;

    if (!plyrdb_module_inited)
    {
        PLYRDB_load_from_disk();
    }

    // does it look like the player db list is empty?
    if (plyrdb_playerdb != NULL)
    {
        list_walk = plyrdb_playerdb;

        while (list_walk != NULL)
        {
            // did we find 'em?
            if (strcmp(name, list_walk->name) == 0)
            {
                // yep!
                return list_walk;
            }

            list_walk = list_walk->next;
        }
    }

    // nope, player list really is empty...
    return list_walk;
}

/****************************************************************************************************************/
/*! \brief Loads the player db from disk.  If the file doesn't exist, it'll try to create it; if this fails, or
 * it has insufficient permissions, it terminates the program (as that's an unrecoverable state).
 * \note The db path is HARD-CODED, and whoever the server is running as MUST have permission to write to,
 * dir list, and read from wherever this gets executed.
 * \todo Accept a cmd line argument that tells us where the db file should live.
 */
void PLYRDB_load_from_disk(void)
{
    if (plyrdb_module_inited) return;

    plyrdb_module_inited = TRUE;

    FILE *fin;

    // The 'b' isn't needed on any Unices, but Windows needs it.

    fin = fopen(PLAYERDB_FILE_PATH, "r+b");

    // were we able to open it?
    if (!fin)
    {
        // no - try creating it...?
        fin = fopen(PLAYERDB_FILE_PATH, "wb");

        // did that work?
        if (!fin)
        {
            // we can't run without the backing file, we're hosed...
            OH_SMEG("\nCouldn't read from or write to player list file!\n \
                    Please check your permissions for the current directory \
                    and try re-running the application.\n");
            exit(1);
        }
        else
        {
            // we can read and write here, but there aren't any players (yet).
            // notify the rest of the world...
            plyrdb_module_inited = TRUE;
            atexit(PLYRDB_cleanup);
            plyrdb_playerdb = NULL;
            fclose(fin);

            // nothing else to do at this point.
            return;
        }
    }
    else
    {
        // we were able to open it - start reading player data.
        // the format is:
        // [0 ............... 29][30....33][34....37][38....41]
        //   name, padded by       Wins     Losses     Ties
        //   NULL bytes        all stored in Motorola byte order
        while (!feof(fin))
        {
            PLAYER_STRUCT *tmp = (PLAYER_STRUCT *)malloc(sizeof(PLAYER_STRUCT));

            if (tmp == NULL)
            {
                // on linux, it turns out you'll NEVER get here, since it'll occasionally lie about
                // how much memory it has left, then cheerfully OOM-kill your process when you actually
                // try to use it :^(

                OH_SMEG("\nWe may have possibly run out of memory while\n \
                        loading the player list from disk.\n \
                        Continuing, but bad things may happen from here on out...");
                fclose(fin);
                return;
            }
            else
            {
                bzero(tmp->name, MAX_NAME_LENGTH);

                // get name
                fgets(tmp->name, MAX_NAME_LENGTH, fin);
                fgetc(fin); // skip padding

                // get player stats (this part is kinda endian-specific)
                tmp->games_won  = (fgetc(fin) << 24) | (fgetc(fin) << 16) | (fgetc(fin) << 8) | (fgetc(fin));
                tmp->games_lost = (fgetc(fin) << 24) | (fgetc(fin) << 16) | (fgetc(fin) << 8) | (fgetc(fin));
                tmp->games_tied = (fgetc(fin) << 24) | (fgetc(fin) << 16) | (fgetc(fin) << 8) | (fgetc(fin));

                tmp->state      = GAMESTATE_NOT_CONNECTED;
            }

            // filled out structure, insert it in the proper place in the list...
            PLYRDB_insert_helper(tmp);

        } // end while not end-of-file

        // should be done by the time we get here, so...
        fclose(fin);
    }

    atexit(PLYRDB_cleanup);
    return;
}

/****************************************************************************************************************/
/*! \brief Serialize and save all players in the list out to disk.
 * \note The db path is HARD-CODED, and whoever the server is running as MUST have permission to write to,
 * dir list, and read from wherever this gets executed.
 * \bug Vulnerable to race condition: someone could change the permissions while the program is running, and
 * screw us up...I have no idea how we'd prevent that...
 * \note We do NOT save the avatar; that's done on the client side.
 */
void PLYRDB_save_to_disk(void)
{
    if (!plyrdb_module_inited)
        PLYRDB_load_from_disk();

    char buf[1024];

    snprintf(buf, 1023, "cp %s %s", PLAYERDB_FILE_PATH, PLAYERDB_BKUP_PATH);
    system(buf);

    FILE *fout;

    PLAYER_STRUCT *list_walk = plyrdb_playerdb;

    fout = fopen(PLAYERDB_FILE_PATH, "wb");

    if (fout == NULL)
    {
        OH_SMEG("Could not write to player db file!\nGonna continue, but saved stats are being lost...");
        return;
    }

    while (list_walk != NULL)
    {
        int tmp;
        fwrite(list_walk->name, MAX_NAME_LENGTH, 1, fout);

        tmp = htonl(list_walk->games_won);
        fwrite(&tmp, sizeof(uint32_t), 1, fout);

        tmp = htonl(list_walk->games_lost);
        fwrite(&tmp, sizeof(uint32_t), 1, fout);

        tmp = htonl(list_walk->games_tied);
        fwrite(&tmp, sizeof(uint32_t), 1, fout);

        list_walk = list_walk->next;
    }

    fflush(fout);
    fclose(fout);
}

/****************************************************************************************************************/
/*! \brief Attempt to add a new player to the player list.
 * \param name The name of the new player to add.
 * \return A pointer to the player.
 */
PLAYER_STRUCT * PLYRDB_create_new_player(const char *name)
{
    PLAYER_STRUCT *tmp;

    // check if this name's in there already
    tmp = PLYRDB_find_by_name(name);

    if (tmp != NULL)
    {
        // player exists already, just return them
        return tmp;
    }

    // if we're here, this player didn't exist yet; create them
    tmp = (PLAYER_STRUCT *)malloc(sizeof(PLAYER_STRUCT));

    // did we have trouble while allocating the player?
    if (tmp == NULL)
    {
        OH_SMEG("failed to allocate %d bytes - the server may encounter problems later...",
            sizeof(PLAYER_STRUCT));
        return FALSE;
    }

    bzero(tmp, sizeof(PLAYER_STRUCT));

    strncpy(tmp->name, name, MAX_NAME_LENGTH - 1);
    PLYRDB_insert_helper(tmp);

    return tmp;
}

/****************************************************************************************************************/
/*! \brief A helper function to append a new player to the list. Should be considered module-private.
 */
static void PLYRDB_insert_helper(PLAYER_STRUCT *tmp)
{
    PLAYER_STRUCT *list_walk        = plyrdb_playerdb;
    PLAYER_STRUCT *list_walk_prev   = plyrdb_playerdb;

    // ...was the list empty?
    if (plyrdb_playerdb == NULL)
    {
        // yep, we get to be first in line :^D
        plyrdb_playerdb =   tmp;
        tmp->next       =   NULL;
        tmp->prev       =   NULL;
    }
    else
    {
        // nope, find correct place
        int name_comparison = (strcmp(tmp->name, ((PLAYER_STRUCT *)list_walk)->name));

        while (list_walk != NULL)   // bug - apparently, Clint's too dumb to sort a linked list on 3 hours of sleep. will fix later :^)
        {
            list_walk_prev = list_walk;
            list_walk = ((PLAYER_STRUCT *)list_walk)->next;
            if (list_walk != NULL) name_comparison = (strcmp(tmp->name, ((PLAYER_STRUCT *)list_walk)->name));

            if (name_comparison == 0)
            {
                OH_SMEG("\nConstraint failure - we shouldn't have gotten asked to insert this player a second time...");
                return;
            }
        }

        tmp->next = NULL;
        tmp->prev = list_walk_prev;
        ((PLAYER_STRUCT *)list_walk_prev)->next = tmp;
    }
}

/****************************************************************************************************************/
/*! \brief Cleanup the player list.  Should be considered module-private and should never be called manually.
 */
void PLYRDB_cleanup(void)
{
    if (!plyrdb_module_inited) return;

    PLYRDB_save_to_disk();

    PLAYER_STRUCT *list_walk        = plyrdb_playerdb;
    PLAYER_STRUCT *list_walk_next   = plyrdb_playerdb;

    while ((list_walk != NULL) && (list_walk_next->next != NULL))
    {
        list_walk_next = list_walk->next;
        free(list_walk);
        list_walk = list_walk_next;
    }
}
