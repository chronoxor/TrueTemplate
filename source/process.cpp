////////////////////////////////////////////////////////////////////////////////
//                                FAR DEBUGGER                                //
//                                                                            //
// AUTHOR:  Shinkarenko Ivan AKA 4ekucT                                       //
// GROUP:   NULL workgroup                                                    //
// PROJECT: FAR plugin - Debugger                                             //
// PART:    Process header                                                    //
// CREATED: 12.01.2004                                                        //
////////////////////////////////////////////////////////////////////////////////
#include "process.h"
/*============================================================================*/
HANDLE OldStdOutput=nullptr;
HANDLE OldStdInput=nullptr;
HANDLE OldStdError=nullptr;
bool IsInstalled=false;
bool ShowFlag=false;
bool ClearFlag=false;
unsigned OutStringsIndex;
/*============================================================================*/
static HANDLE PipeStdOutputDup=nullptr;
static HANDLE PipeStdInputDup=nullptr;
static HANDLE PipeStdErrorDup=nullptr;
static HANDLE StdOutputThread=nullptr;
static HANDLE StdInputThread=nullptr;
static HANDLE StdErrorThread=nullptr;
static DWORD WINAPI StdOutputThreadFunction(LPVOID lpParameter);
static DWORD WINAPI StdInputThreadFunction(LPVOID lpParameter);
static DWORD WINAPI StdErrorThreadFunction(LPVOID lpParameter);
/*============================================================================*/
static DWORD WINAPI StdOutputThreadFunction(LPVOID lpParameter)
{
 BOOL OkFlag;
 UCHAR Buffer[4096];
 DWORD ReadCount,Temp;
 while (!IsInstalled)
  SwitchToThread();
 if (ClearFlag)
  ClearScreen();
 while (true)
 {
  OkFlag=ReadFile(PipeStdOutputDup,Buffer,4096,&ReadCount,nullptr);
  if ((OkFlag)&&(ReadCount>0))
  {
   if (ShowFlag)
    WriteFile(OldStdOutput,Buffer,ReadCount,&Temp,nullptr);
  }
  SwitchToThread();
 }
 return 0;
}
/*----------------------------------------------------------------------------*/
static DWORD WINAPI StdInputThreadFunction(LPVOID lpParameter)
{
 BOOL OkFlag;
 UCHAR Buffer[4096];
 DWORD ReadCount,WriteCount;
 while (!IsInstalled)
  SwitchToThread();
 while (true)
 {
  OkFlag=ReadFile(OldStdInput,Buffer,4096,&ReadCount,nullptr);
  if (ReadCount>0)
   WriteFile(PipeStdInputDup,Buffer,ReadCount,&WriteCount,nullptr);
  SwitchToThread();
 }
 return 0;
}
/*----------------------------------------------------------------------------*/
static DWORD WINAPI StdErrorThreadFunction(LPVOID lpParameter)
{
 BOOL OkFlag;
 UCHAR Buffer[4096];
 DWORD ReadCount,Temp;
 while (!IsInstalled)
  SwitchToThread();
 while (true)
 {
  OkFlag=ReadFile(PipeStdErrorDup,Buffer,4096,&ReadCount,nullptr);
  if ((OkFlag)&&(ReadCount>0))
  {
   if (ShowFlag)
    WriteFile(OldStdError,Buffer,ReadCount,&Temp,nullptr);
  }
  SwitchToThread();
 }
 return 0;
}
/*============================================================================*/
// METHOD: Clear console
void ClearScreen(void)
{
 CONSOLE_SCREEN_BUFFER_INFO csbi;
 GetConsoleScreenBufferInfo(OldStdOutput,&csbi);
 DWORD dummy;
 COORD C;
 C.X=0;
 for (int Y=0;Y<csbi.dwSize.Y;Y++)
 {
  C.Y=(short)Y;
  FillConsoleOutputCharacter(OldStdOutput,L' ',csbi.dwSize.X,C,&dummy);
  FillConsoleOutputAttribute(OldStdOutput,7,csbi.dwSize.X,C,&dummy);
 }
 C.Y=(short)(csbi.dwSize.Y-1);
 SetConsoleCursorPosition(OldStdOutput,C);
}
/*----------------------------------------------------------------------------*/
// METHOD: Install system
// RETURN:
//     1) true  - if installed
//     2) false - if wasn't install
bool ProcessInstall(void)
{
 if (!IsInstalled)
 {
  OldStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
  OldStdInput=GetStdHandle(STD_INPUT_HANDLE);
  OldStdError=GetStdHandle(STD_ERROR_HANDLE);
  BOOL CreateStdOutput,CreateStdInput,CreateStdError;
  HANDLE PipeStdOutputWrite,PipeStdOutputRead;
  HANDLE PipeStdInputWrite,PipeStdInputRead;
  HANDLE PipeStdErrorWrite,PipeStdErrorRead;
  SECURITY_ATTRIBUTES saAttrStdOutput,saAttrStdInput,saAttrStdError;
  saAttrStdOutput.nLength=sizeof(SECURITY_ATTRIBUTES);
  saAttrStdOutput.bInheritHandle=true;
  saAttrStdOutput.lpSecurityDescriptor=nullptr;
  CreateStdOutput=CreatePipe(&PipeStdOutputRead,&PipeStdOutputWrite,&saAttrStdOutput,0);
  saAttrStdInput.nLength=sizeof(SECURITY_ATTRIBUTES);
  saAttrStdInput.bInheritHandle=true;
  saAttrStdInput.lpSecurityDescriptor=nullptr;
  CreateStdInput=CreatePipe(&PipeStdInputRead,&PipeStdInputWrite,&saAttrStdInput,0);
  saAttrStdError.nLength=sizeof(SECURITY_ATTRIBUTES);
  saAttrStdError.bInheritHandle=true;
  saAttrStdError.lpSecurityDescriptor=nullptr;
  CreateStdError=CreatePipe(&PipeStdErrorRead,&PipeStdErrorWrite,&saAttrStdError,0);
  if ((!CreateStdOutput)||(!CreateStdInput)||(!CreateStdError))
  {
   ProcessUninstall();
   return false;
  }
  CreateStdOutput=SetStdHandle(STD_OUTPUT_HANDLE,PipeStdOutputWrite);
  CreateStdInput=SetStdHandle(STD_INPUT_HANDLE,PipeStdInputRead);
  CreateStdError=SetStdHandle(STD_ERROR_HANDLE,PipeStdErrorWrite);
  if ((!CreateStdOutput)||(!CreateStdInput)||(!CreateStdError))
  {
   ProcessUninstall();
   return false;
  }
  CreateStdOutput=DuplicateHandle(GetCurrentProcess(),PipeStdOutputRead,GetCurrentProcess(),&PipeStdOutputDup,0,FALSE,DUPLICATE_SAME_ACCESS);
  CreateStdInput=DuplicateHandle(GetCurrentProcess(),PipeStdInputWrite,GetCurrentProcess(),&PipeStdInputDup,0,FALSE,DUPLICATE_SAME_ACCESS);
  CreateStdError=DuplicateHandle(GetCurrentProcess(),PipeStdErrorRead,GetCurrentProcess(),&PipeStdErrorDup,0,FALSE,DUPLICATE_SAME_ACCESS);
  if ((!CreateStdOutput)||(!CreateStdInput)||(!CreateStdError))
  {
   ProcessUninstall();
   return false;
  }
  CreateStdOutput=CloseHandle(PipeStdOutputRead);
  CreateStdInput=CloseHandle(PipeStdInputWrite);
  CreateStdError=CloseHandle(PipeStdErrorRead);
  if ((!CreateStdOutput)||(!CreateStdInput)||(!CreateStdError))
  {
   ProcessUninstall();
   return false;
  }
  DWORD StdOutputThreadID;
  DWORD StdInputThreadID;
  DWORD StdErrorThreadID;
  StdOutputThread=CreateThread(nullptr,0,(LPTHREAD_START_ROUTINE)StdOutputThreadFunction,nullptr,0,&StdOutputThreadID);
  StdInputThread=CreateThread(nullptr,0,(LPTHREAD_START_ROUTINE)StdInputThreadFunction,nullptr,0,&StdInputThreadID);
  StdErrorThread=CreateThread(nullptr,0,(LPTHREAD_START_ROUTINE)StdErrorThreadFunction,nullptr,0,&StdErrorThreadID);
  if ((StdOutputThread==nullptr)||(StdInputThread==nullptr)||(StdErrorThread==nullptr))
  {
   ProcessUninstall();
   return false;
  }
  IsInstalled=true;
  return true;
 }
 else
  return false;
}
/*----------------------------------------------------------------------------*/
// METHOD: Uninstall system
// RETURN:
//     1) true  - if uninstalled
//     2) false - if wasn't uninstall
bool ProcessUninstall(void)
{
 if (IsInstalled)
 {
  if (StdOutputThread!=nullptr)
   TerminateThread(StdOutputThread,0);
  if (StdInputThread!=nullptr)
   TerminateThread(StdInputThread,0);
  if (StdErrorThread!=nullptr)
   TerminateThread(StdErrorThread,0);
  HANDLE CloseStdOutput,CloseStdInput,CloseStdError;
  CloseStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
  CloseStdInput=GetStdHandle(STD_INPUT_HANDLE);
  CloseStdError=GetStdHandle(STD_ERROR_HANDLE);
  if (CloseStdOutput!=nullptr)
   CloseHandle(CloseStdOutput);
  if (CloseStdInput!=nullptr)
   CloseHandle(CloseStdInput);
  if (CloseStdError!=nullptr)
   CloseHandle(CloseStdError);
  SetStdHandle(STD_OUTPUT_HANDLE,OldStdOutput);
  SetStdHandle(STD_INPUT_HANDLE,OldStdInput);
  SetStdHandle(STD_ERROR_HANDLE,OldStdError);
  if (PipeStdOutputDup!=nullptr)
   CloseHandle(PipeStdOutputDup);
  if (PipeStdInputDup!=nullptr)
   CloseHandle(PipeStdInputDup);
  if (PipeStdErrorDup!=nullptr)
   CloseHandle(PipeStdErrorDup);
  IsInstalled=false;
  return true;
 }
 else
  return false;
}
/*============================================================================*/
