#ifndef __TCOLL_H
#define __TCOLL_H

struct TCollectionItem
{
	virtual ~TCollectionItem()
	{
	}
};

class TCommonCollection
{
public:
	typedef bool(*FindProc)(const TCollectionItem*, void*);

	TCommonCollection(size_t, size_t);
	TCommonCollection();
	virtual ~TCommonCollection();
	TCollectionItem *operator[](size_t i) { return at(i); };
	const TCollectionItem *operator[](size_t i) const { return at(i); };
	size_t insert(TCollectionItem*);
	void removeAll(void);
	size_t remove(size_t);
	virtual void setLimit(size_t);
	size_t getCount() const { return count; }
	TCollectionItem *find(FindProc, void*);
	ptrdiff_t findIndex(FindProc, void*);

protected:
	TCollectionItem *at(size_t);
	const TCollectionItem *at(size_t) const;
	TCollectionItem **items;
	size_t count, limit, delta;
	void init(size_t, size_t);

private:
	TCommonCollection(const TCommonCollection &coll) {}
	TCommonCollection& operator=(const TCommonCollection &coll) {}
};

template<class T>
class TCollection : public TCommonCollection
{
public:
	typedef bool(*FindProc)(const T*, void*);

	T *operator[](size_t i) { return at(i); };
	const T *operator[](size_t i) const { return at(i); };
	size_t insert(T *item) { return TCommonCollection::insert(item); }
	T *find(FindProc ff, void *data) {
		TCommonCollection::FindProc fc = reinterpret_cast<TCommonCollection::FindProc>(ff);
		return reinterpret_cast<T*>(TCommonCollection::find(fc, data));
	}
	ptrdiff_t findIndex(FindProc ff, void *data) {
		TCommonCollection::FindProc fc = reinterpret_cast<TCommonCollection::FindProc>(ff);
		return TCommonCollection::findIndex(fc, data);
	}

protected:
	size_t insert(TCollectionItem*);

	T *at(size_t i) { return static_cast<T*>(TCommonCollection::at(i)); }
	const T *at(size_t i) const { return static_cast<const T*>(TCommonCollection::at(i)); };
};

#endif
