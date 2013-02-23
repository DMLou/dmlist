/*****************************************************************************

  Updates.h

  Lets ListXP check for updates, along with a possible custom message file
  detailing the new updates.

*****************************************************************************/

#ifndef _UPDATES_H
#define _UPDATES_H


#include <windows.h>
#include "ListTypes.h"
#include "List.h"


// Simply call this function to see if there is an update available. It will
// return, if necessary, the latest version + its special update message text
// Return value: true if successful, false if some error
//               LatestVersion set to new version text (i.e. "0.87")
//               UpdateMessage set to updateinfo.txt's "Message=" line (w/o the Message= part though)
extern bool GetUpdateInfo (string &LatestVersion, string &UpdateMessage);

// Pops up a dialog asking for user action when a new version is available
// It will ask the user if they want to go to the website or remind them later.
// In the former case, the Windows shell will be called to open a URL. Also,
// a checkbox is presented to allow the user to disable updates (controls the
// "UpdatesEnabled" var).
extern void BugUserAboutUpdate (HWND Parent, ListThreadContext *LC, const string &NewVersion, const string &UpdateMessage);

// Call this to spawn an update info check
// if OverrideEnable is enabled, then the check will be done regardless of the UpdatesEnabled
// global variable
// if Async is true then the check will be performed asynchronously (i.e. separate thread)
// if ASync is false, then function will not return until the update check is complete
// and then the bool value pointed to by FoundUpdateResult (if non-NULL) will be set to
// true or false depending on whether an update was found
// and the strings in InfoResult, if it is non-NULL, will contain the proper information
// if an update was found.
// This does all the time checking as well, so it will make sure that N days have elapsed
// before the last check. And it will set the UpdatesLastChecked date when successful
class ListThreadContext;
extern void CheckForUpdate (ListThreadContext *LC,
                            bool OverrideEnable = false, 
                            bool Async = true, 
                            bool *FoundUpdateResult = NULL,
                            UpdateInfo *InfoResult = NULL);


// Returns an integer telling the difference between two times, in milliseconds
// effectively lhs-rhs
extern sint64 TimeDiffMS (string &lhs, string &rhs);

// Returns an integer telling the difference between two times, in days
extern sint64 TimeDiffDays (string &lhs, string &rhs);


#endif // _UPDATES_H
