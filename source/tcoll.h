#ifndef __TCOLL_H
#define __TCOLL_H

struct TCollectionItem
{
	virtual ~TCollectionItem()
	{
	}
};

class TCollection
{
public:
	TCollection(size_t, size_t);
	TCollection();
	virtual ~TCollection();
	void init(size_t = 0, size_t = 5);
	TCollectionItem *operator[](size_t i) { return at(i); };
	size_t insert(TCollectionItem*);
	void removeAll(void);
	size_t remove(size_t);
	virtual void setLimit(size_t);
	size_t getCount() const { return count; }
	TCollectionItem *find(bool(*)(TCollectionItem*, void*), void*);
	ptrdiff_t findIndex(bool(*)(TCollectionItem*, void*), void*);
protected:
	TCollectionItem *at(size_t);
	TCollectionItem **items;
	size_t count, limit, delta;
private:
	TCollection(const TCollection &coll) {}
	TCollection& operator=(const TCollection &coll) {}
};
#endif
