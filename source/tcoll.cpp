////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"
#include "tcoll.h"

static const size_t MAXCOLLECTIONSIZE = 0x7FFFFFFF;

TCollection::TCollection(size_t aLimit, size_t aDelta)
{
	init(aLimit, aDelta);
}

TCollection::TCollection()
{
	init(0, 5);
}

void TCollection::init(size_t aLimit, size_t aDelta)
{
	count = limit = 0;
	items = nullptr;
	delta = aDelta;
	setLimit(aLimit);
}

TCollection::~TCollection()
{
	removeAll();
	delete[] items;
}

TCollectionItem *TCollection::at(size_t index)
{
	if (index >= count)
		return nullptr;
	return items[index];
}

TCollectionItem *TCollection::find(bool(*ff)(TCollectionItem*, void*), void *data)
{
	if (items)
	{
		for (size_t i = 0; i < count; i++)
			if (items[i] && ff(items[i], data))
				return items[i];
	}
	return nullptr;
}

ptrdiff_t TCollection::findIndex(bool(*ff)(TCollectionItem*, void*), void *data)
{
	if (items)
	{
		for (size_t i = 0; i < count; i++)
			if (items[i] && ff(items[i], data))
				return i;
	}
	return -1;
}

size_t TCollection::insert(TCollectionItem *item)
{
	if (count == limit)
		setLimit(count + delta);
	items[count] = item;
	return count++;
}

size_t TCollection::remove(size_t index)
{
	if (items && (index < limit) && count)
	{
		if (items[index])
			delete items[index];
		memcpy(items + index, items + index + 1, (--count - index)*sizeof(items[0]));
	}
	return count;
}

void TCollection::removeAll(void)
{
	if (items)
	{
		for (size_t i = 0; i < count; i++)
		{
			if (items[i])
			{
				delete items[i];
				items[i] = nullptr;
			}
		}
	}
	setLimit(count = 0);
}

void TCollection::setLimit(size_t aLimit)
{
	if (aLimit < count)
		aLimit = count;
	if (aLimit > MAXCOLLECTIONSIZE)
		aLimit = MAXCOLLECTIONSIZE;
	if (aLimit != limit)
	{
		TCollectionItem **aItems;
		if (aLimit == 0)
			aItems = nullptr;
		else
		{
			aItems = new TCollectionItem*[aLimit];
			if (!aItems)
				return;
			if (count != 0)
				memcpy(aItems, items, count*sizeof(TCollectionItem*));
		}
		if (items != nullptr)
			delete[] items;
		items = aItems;
		limit = aLimit;
	}
}
