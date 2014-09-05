typedef union SCharData
{
	long IArr[8];  // must be 32 bytes!!!
	char CArr[32];
	void setbit(unsigned char Bit);
	void clearbit(unsigned char Bit);
	bool getbit(unsigned char Bit);
} *PCharData;

class CLocale
{
public:
  bool static cl_isdigit(wchar_t c);
  bool static cl_isword(wchar_t c);
  bool static cl_isuppercase(wchar_t c);
  bool static cl_islowercase(wchar_t c);
  wchar_t static cl_lowcase(wchar_t c);
  wchar_t static cl_upcase(wchar_t c);
  int static cl_stricmp(wchar_t *c1, wchar_t *c2);
  int static cl_strnicmp(wchar_t *c1, wchar_t *c2, size_t len);
};

////////////////////////////////////////////////////////////////////////////
// bit ops - inline (good speedup)
void inline SCharData::setbit(unsigned char Bit)
{
	CArr[Bit >> 3] |= (char)(1 << (Bit&7));
};
void inline SCharData::clearbit(unsigned char Bit)
{
	CArr[Bit >> 3] &= (char)(~(1 << (Bit&7)));
};
bool inline SCharData::getbit(unsigned char Bit)
{
	return (CArr[Bit >> 3] & (1 << (Bit&7))) != 0;
};
