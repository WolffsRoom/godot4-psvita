/*
 * SDK shim — VitaSDK header/stub mismatch workaround.
 *
 * This VitaSDK ships <psp2/appmgr.h> declaring `sceAppMgrGetBudgetInfo()`, but
 * its SceAppMgr stub (and the NID db) only export the raw NID symbol
 * `_sceAppMgrGetBudgetInfo`. Recent VitaGL calls the non-underscore name, so it
 * fails to link. Bridge the two here.
 *
 * If you regenerate/update the SDK stubs so the wrapper exists, delete this file.
 */
#include <psp2/appmgr.h>

extern int _sceAppMgrGetBudgetInfo(SceAppMgrBudgetInfo *info);

int sceAppMgrGetBudgetInfo(SceAppMgrBudgetInfo *info) {
	return _sceAppMgrGetBudgetInfo(info);
}
