#ifndef __TRUE_TPL_H
#define __TRUE_TPL_H

#define PROCESS_EVENT 0
#define IGNORE_EVENT  1

#define NM 4096

#endif

enum TCOMMAND
{
    CMD_Argument,
    CMD_ClipBoard,
    CMD_Counter,
    CMD_CompName,
    CMD_Date,
    CMD_DateTime,
    CMD_Decrement,
    CMD_Enviroment,
    CMD_Exec,
    CMD_File,
    CMD_FileUp,
    CMD_FileExt,
    CMD_FileExtUp,
    CMD_FileName,
    CMD_FileNameUp,
    CMD_FileNameExt,
    CMD_FileNameExtUp,
    CMD_FilePath,
    CMD_FilePathUp,
    CMD_GUID,
    CMD_Increment,
    CMD_Input,
    CMD_InputString,
    CMD_OsName,
    CMD_OsType,
    CMD_Random,
    CMD_Selected,
    CMD_String,
    CMD_Ticks,
    CMD_Time,
    CMD_UserName,
    CMD_EditorString,
    CMD_EditorPos,
    CMD_EditorCol,
//--------------------------------------------------
    CMD_Unknown,
    CMD_DoNothing
};
