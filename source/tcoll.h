#ifndef __TCOLL_H
#define __TCOLL_H

class TCollection
{
public:
	TCollection(size_t, size_t, void(*)(void*));
	TCollection();
	virtual ~TCollection();
	void init(size_t = 0, size_t = 5, void(*)(void*) = 0);
	void done(void);
	inline void *operator[](size_t i) { return at(i); };
	size_t insert(void*);
	void removeAll(void);
	size_t remove(size_t);
	virtual void setLimit(size_t);
	size_t getCount() { return count; }
	void *find(bool(*)(void*, void*), void*);
	ptrdiff_t findIndex(bool(*)(void*, void*), void*);
protected:
	void *at(size_t);
	void **items;
	size_t count, limit, delta;
	void(*delItem)(void*);
private:
	TCollection(const TCollection &coll) {}
	TCollection& operator=(const TCollection &coll) {}
};
#endif
