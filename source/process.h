////////////////////////////////////////////////////////////////////////////////
//                                FAR DEBUGGER                                //
//                                                                            //
// AUTHOR:  Shinkarenko Ivan AKA 4ekucT                                       //
// GROUP:   NULL workgroup                                                    //
// PROJECT: FAR plugin - Debugger                                             //
// PART:    Process header                                                    //
// CREATED: 12.01.2004                                                        //
////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
/*============================================================================*/
extern HANDLE OldStdOutput;
extern HANDLE OldStdInput;
extern HANDLE OldStdError;
extern bool IsInstalled;
extern bool ShowFlag;
extern bool ClearFlag;
extern unsigned OutStringsIndex;
/*============================================================================*/
// METHOD: Clear console
void ClearScreen(void);
// METHOD: Install system
// RETURN:
//     1) true  - if installed
//     2) false - if wasn't install
bool ProcessInstall(void);
// METHOD: Uninstall system
// RETURN:
//     1) true  - if uninstalled
//     2) false - if wasn't uninstall
bool ProcessUninstall(void);
/*============================================================================*/
