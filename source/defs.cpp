struct TIndent
{
	wchar_t		mask[MAX_REG_LEN], relative[MAX_REG_LEN], immChar;
	intptr_t	indent[2], start;
	size_t		bracketLink;
	int				BracketsMode;
};

struct TBracket
{
	wchar_t	open[MAX_REG_LEN], close[MAX_REG_LEN];
};

struct TComment
{
	wchar_t	mask[MAX_REG_LEN];
};

struct TDefine
{
	TDefine (const wchar_t *, const wchar_t *);
	wchar_t	name[MAX_REG_LEN], value[MAX_STR_LEN];
};

TDefine::TDefine (const wchar_t *aName, const wchar_t *aValue)
{
	wcscpy (name, aName);
	wcscpy (value, aValue);
}

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
