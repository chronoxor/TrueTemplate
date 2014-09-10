class TIndent : public TCollectionItem
{
public:
	String		mask, relative;
	wchar_t		immChar;
	intptr_t	indent[2], start;
	size_t		bracketLink;
	int				BracketsMode;
};

class TBracket : public TCollectionItem
{
public:
	String	open, close;
};

class TComment : public TCollectionItem
{
public:
	String	mask;
};

class TDefine : public TCollectionItem
{
public:
	String	name, value;
	TDefine(const wchar_t *aName, const wchar_t *aValue)
		: name(aName), value(aValue)
	{
	}
};

enum TJumpType
{
	jtNone,
	jtFirst,
	jtMenu,
	jtSmart
};
enum TSaveType
{
	saNone,
	saAll,
	saCurrent
};
enum TCDType
{
	cdNone,
	cdFile,
	cdBase
};
