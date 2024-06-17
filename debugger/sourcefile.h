
#ifndef SOURCEFILE_H
#define SOURCEFILE_H

class SourceFile : public CRichEditCtrl{
public:
	SourceFile();
	~SourceFile();

	void highLight( int row,int col );
	void setText(istream& in);

	DECLARE_DYNAMIC( SourceFile )
	DECLARE_MESSAGE_MAP()

	afx_msg int  OnCreate( LPCREATESTRUCT lpCreateStruct );

private:
	string is_line;
	istream *is_stream;
	int is_curs,is_linenum;

	void formatStreamLine();
	DWORD streamIn( LPBYTE buff,LONG cnt,LONG *done );
};

#endif
