#ifndef __TCOLL_H
#define __TCOLL_H

class TCollection
{
  public:
    TCollection(unsigned, unsigned, void (*)(void*));
    TCollection();
    TCollection(const TCollection &coll);
    TCollection& operator=(const TCollection &coll);
    virtual ~TCollection();
    void init(unsigned = 0, unsigned = 5, void (*)(void*) = 0);
    void done(void);
    inline void *operator[](unsigned i) { return at(i);};
    unsigned insert(void*);
    void removeAll(void);
    unsigned remove(unsigned);
    virtual void setLimit(unsigned);
    unsigned getCount() { return count; }
    void *find(int (*)(void*, void*), void*);
    int findIndex(int (*)(void*, void*), void*);
  protected:
    void *at(unsigned);
    void **items;
    unsigned count, limit, delta;
    void (*delItem)(void*);
};
#endif
