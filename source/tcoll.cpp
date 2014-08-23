////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"
#include "tcoll.h"

#define MAXCOLLECTIONSIZE 0x7FFFFFFF

TCollection::TCollection(unsigned aLimit, unsigned aDelta, void (*aDelItem)(void*))
{
  init(aLimit, aDelta, aDelItem);
}

TCollection::TCollection()
{
  init(0, 5, NULL);
}

void TCollection::init(unsigned aLimit, unsigned aDelta, void (*aDelItem)(void*))
{
  count = limit = 0;
  items = NULL;
  delta = aDelta;
  delItem = aDelItem;
  setLimit(aLimit);
}

TCollection::~TCollection()
{
  done();
}

void TCollection::done(void)
{
  removeAll();
  if ( items )
  {
    delete [] items;
    items = NULL;
  }
}

void *TCollection::at(unsigned index)
{
  if ( index >= count )
    return NULL;
  return items[index];
}

void *TCollection::find(int (*ff)(void*, void*), void *data)
{
  if ( items )
  {
    for ( unsigned i =  0; i < count; i++ )
      if ( items[i] && ff(items[i], data) )
        return items[i];
  }
  return NULL;
}

int TCollection::findIndex(int (*ff)(void*, void*), void *data)
{
  if ( items )
  {
    for ( unsigned i =  0; i < count; i++ )
      if ( items[i] && ff(items[i], data) )
        return i;
  }
  return -1;
}

unsigned TCollection::insert(void *item)
{
  if ( count == limit )
    setLimit(count+delta);
  items[count] = item;
  return count++;
}

unsigned TCollection::remove(unsigned index)
{
  if ( items && ( index < limit ) && count )
  {
    if ( items[index] )
      if ( delItem )
        delItem(items[index]);
      else
        delete items[index];
    memcpy(items+index, items+index+1, (--count-index)*sizeof(items[0]));
  }
  return count;
}

void TCollection::removeAll(void)
{
  if ( items )
  {
    for ( unsigned i = 0 ; i < count ; i++ )
      if ( items[i] )
      {
        if ( delItem )
          delItem(items[i]);
        else
          delete items[i];
        items[i] = NULL;
      }
  }
  setLimit(count = 0);
}

void TCollection::setLimit(unsigned aLimit)
{
  if ( aLimit < count )
    aLimit =  count;
  if ( aLimit > MAXCOLLECTIONSIZE)
    aLimit = MAXCOLLECTIONSIZE;
  if ( aLimit != limit )
  {
    void **aItems;
    if ( aLimit == 0 )
      aItems = NULL;
    else
    {
      aItems = new void*[aLimit];
      if ( !aItems )
        return;
      if ( count != 0 )
        memcpy(aItems, items, count*sizeof(void*));
    }
    if ( items != NULL )
      delete [] items;
    items = aItems;
    limit = aLimit;
  }
}
