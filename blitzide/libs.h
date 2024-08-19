
#ifndef LIBS_H
#define LIBS_H

extern int compiler_ver,linker_ver,runtime_ver;

void	initLibs();
string	quickHelp( const string &kw );
bool	isMediaFile( const string &file );

#endif
