////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"
#include "guid.h"
#include "FARIntf.h"

struct PluginStartupInfo Info;
struct FarStandardFunctions FSF;
HANDLE RegExpHandle;
wchar_t PluginRootKey[80];

void InitDialogItems(InitDialogItem *Init, FarDialogItem *Item, size_t ItemsNumber)
{
  for (size_t I = 0 ; I < ItemsNumber ; I++ )
  {
    Item[I].Type = (FARDIALOGITEMTYPES)Init[I].Type;
    Item[I].X1 = Init[I].X1;
    Item[I].Y1 = Init[I].Y1;
    Item[I].X2 = Init[I].X2;
    Item[I].Selected = Init[I].Selected;
    Item[I].Flags = Init[I].Flags;
	Item[I].Flags &= ~(DIF_FOCUS | DIF_DEFAULTBUTTON);
    if ( Init[I].Y2 == 255 )
    {
      Item[I].Y2 = 0;
	  Item[I].Data = L"";
      Item[I].Flags = DIF_BOXCOLOR|DIF_SEPARATOR;
    }
    else
    {
      Item[I].Y2 = Init[I].Y2;
      if ( (unsigned int)Init[I].Data < 2000 )
        Item[I].Data = GetMsg((unsigned int)Init[I].Data);
      else
        Item[I].Data = Init[I].Data;
    }
	Item[I].MaxLength = 0;
	Item[I].History = nullptr;
	Item[I].Mask = nullptr;
	Item[I].UserData = 0;
	Item[I].Reserved[0] = 0;
	Item[I].Reserved[1] = 0;
  }
}

void InitDialogItemsEx(const struct InitDialogItemEx *Init, struct FarDialogItem *Item, size_t ItemsNumber)
{
 struct FarDialogItem *PItem=Item;
 const struct InitDialogItemEx *PInit=Init;
 for (size_t I = 0; I < ItemsNumber; I++, PItem++, PInit++)
 {
  PItem->Type=(FARDIALOGITEMTYPES)PInit->Type;
  PItem->X1=PInit->X1;
  PItem->Y1=PInit->Y1;
  PItem->X2=PInit->X2;
  PItem->Y2=PInit->Y2;
  PItem->Flags=PInit->Flags;
  PItem->Flags |= PInit->Focus ? DIF_FOCUS : 0;
  PItem->Selected=PInit->Selected;
  PItem->History=nullptr;
  if (PInit->History!=nullptr)
   PItem->History=PInit->History;
  PItem->Flags |= PInit->DefaultButton ? DIF_DEFAULTBUTTON : 0;
  if ((unsigned int)PInit->Data<2000)
   PItem->Data = GetMsg((unsigned int)PInit->Data);
  else
   PItem->Data = PInit->Data;
  PItem->MaxLength = 0;
  PItem->Mask = nullptr;
  PItem->UserData = 0;
  PItem->Reserved[0] = 0;
  PItem->Reserved[1] = 0;
 }
}

static void SaveDialogItems(FarDialogItem *Item, InitDialogItem *Init, int ItemsNumber)
{
  for (int I = 0 ; I < ItemsNumber ; I++ )
  {
    Init[I].Selected = (unsigned int)Item[I].Selected;
    if ( (unsigned int)Init[I].Data >= 2000 && Init[I].Y2 != 255 )
      wcscpy(Init[I].Data, Item[I].Data);
  }
}
