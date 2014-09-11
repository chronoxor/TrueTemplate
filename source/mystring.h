#ifndef __MYSTRING_H
#define __MYSTRING_H

#include <wchar.h>

class String
{
public:
	String() : m_buf(nullptr), m_len(0) 
	{
		clear();
	}

	String(const wchar_t *p) : m_buf(nullptr), m_len(0)
	{
		*this = p;
	}

	String(const wchar_t *begin, const wchar_t *end)
	{
		if (begin <= end)
		{
			size_t len = end - begin;
			m_buf = new wchar_t[len + 1];
			m_len = len;
			wcsncpy(m_buf, begin, len);
			m_buf[len] = 0;
		}
		else
			clear();
	}

	String(const String &rhs) : m_buf(nullptr), m_len(0)
	{
		*this = rhs;
	}

	~String()
	{
		delete[] m_buf;
	}

	operator const wchar_t*() const { return m_buf; }

	String &operator=(const wchar_t *p)
	{
		if (p != nullptr)
		{
			size_t len = wcslen(p);
			wchar_t *newbuf = new wchar_t[len + 1];
			wcscpy(newbuf, p);
			delete[] m_buf;
			m_buf = newbuf;
			m_len = len;
		}
		else
			clear();
		return *this;
	}

	String &operator=(const String &rhs)
	{
		if (this != &rhs)
		{
			delete[] m_buf;
			m_buf = new wchar_t[rhs.m_len + 1];
			wcscpy(m_buf, rhs.m_buf);
			m_len = rhs.m_len;
		}
		return *this;
	}
	
	String &operator+=(wchar_t c)
	{
		wchar_t *newbuf = new wchar_t[m_len + 2];
		wcscpy(newbuf, m_buf);
		newbuf[m_len] = c;
		newbuf[m_len + 1] = L'\0';
		delete[] m_buf;
		m_buf = newbuf;
		m_len++;
		return *this;
	}

	String &operator+=(const String &rhs)
	{
		wchar_t *newbuf = new wchar_t[m_len + rhs.m_len + 1];
		wcscpy(newbuf, m_buf);
		wcscpy(newbuf + m_len, rhs.m_buf);
		newbuf[m_len + rhs.m_len] = L'\0';
		delete[] m_buf;
		m_buf = newbuf;
		m_len += rhs.m_len;
		return *this;
	}

	size_t length() const { return m_len; }
	bool empty() const { return m_len == 0; }

	void clear()
	{
		m_len = 0;
		delete[] m_buf;
		m_buf = new wchar_t[1];
		m_buf[0] = L'\0';
	}

private:
	wchar_t *m_buf;
	size_t m_len;
};


#endif
