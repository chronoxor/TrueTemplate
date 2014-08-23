struct TLang
{
	TLang();
	wchar_t				mask[MAX_REG_LEN], desc[MAX_REG_LEN], imm[MAX_REG_LEN], immExp[MAX_REG_LEN], blockcomment[MAX_REG_LEN];
	bool				ignoreCase;
	int					setCP;
	TExec				defExec;
	TCollection macroColl, indentColl, bracketColl, commentColl, execColl, compilerColl, defineColl, navyColl,
		formatColl;
};

TLang::TLang ()
{
	wcscpy (blockcomment, L"");
	defineColl.insert (new TDefine (L"lt", L"<"));
	defineColl.insert (new TDefine (L"gt", L">"));
	defineColl.insert (new TDefine (L"quot", L"\""));
	defineColl.insert (new TDefine (L"amp", L"&"));
	defineColl.insert (new TDefine (L"nbsp", L" "));
}

static void delLang (void *it)
{
	TLang *lng = (TLang *) it;
	lng->macroColl.removeAll ();
	lng->indentColl.removeAll ();
	lng->bracketColl.removeAll ();
	lng->commentColl.removeAll ();
	lng->execColl.removeAll ();
	lng->compilerColl.removeAll ();
	lng->defineColl.removeAll ();
	lng->navyColl.removeAll ();
	lng->formatColl.removeAll ();
}

class TLangCollection : public TCollection
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	public:

		TLangCollection () :
		TCollection(5, 5, delLang)
		{
		};
};

static TLangCollection	langColl;
