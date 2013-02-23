#ifndef _SETTINGS_H
#define _SETTINGS_H


#include "List.h"
#include "VarList.h"
#include "ListRegistry.h"



extern void ApplySettingsV2 (ListThreadContext *LC, VarListExt &NewSettings);
extern void SaveSettingsV2 (ListThreadContext *LC);


/*
extern void LoadSettings (ListThreadContext *LC);
extern void SaveSettings (ListThreadContext *LC);
extern void SetToDefaults (ListSettings *LS);
extern bool ValidateSettings (ListSettings *LS);
extern bool AreSettingsEqual (ListSettings *lhs, ListSettings *rhs);
*/


extern bool GetShellIntegration (void);
extern bool SetShellIntegration (bool Enabled);


#endif // _SETTINGS_H
