/****************************************************************************
** $Id$
**
** This is a utility program for merging findtr msgfiles
**
**
** Copyright (C) 1998 by Trolltech AS.  All rights reserved.
**
*****************************************************************************/
#include <qfile.h>
#include <qbuffer.h>
#include <qtextstream.h>

#include <stdio.h>
#include <stdlib.h>

bool isEmpty( const QString& line )
{
    int i = 0;
    while ( i < int(line.length()) ) {
	if ( !line[i].isSpace() )
	    return FALSE;
	i++;
    }
    return TRUE;
}
	

bool hasHandle( const QString& line, const QString& handle)
{
    return line.left(handle.length()) == handle;
}

bool isUserComment( const QString& line )
{
    return line.length() > 2 && line[0] == '#' && line[1].isSpace();
}

bool isSystemComment( const QString& line )
{
    return line.length() > 1 && line[0] == '#' && !line[1].isSpace();
}


QString extractContents( const QString& line )
{
    QString contents;
    int pos = 0;
    bool inStr = FALSE;
    while ( pos < int(line.length())  ) {
	if ( line[pos] == '"' ) {
	    inStr = !inStr;
	    pos++;
	} else {
	    if ( inStr ) {
		if ( line[pos] == '\\') {
		    pos++;
		    switch (char(line[pos]) ) {
		    case 'n':
			contents += '\n';
			break;
		    case 't':
			contents += '\t';
			break;
		    case 'r':
			contents += '\r';
			break;
		    case 'a':
			contents += '\a';
			break;
		    case 'f':
			contents += '\f';
			break;
		    case 'v':
			contents += '\v';
			break;
		    case 'b':
			contents += '\b';
			break;
		    default:
			contents += char(line[pos]);
			break;
		    }
		} else {
 		    contents += line[pos];
		}
	    }
	    pos++;
	}
    }
    return contents;
}

struct Item
{
    Item() {}
    Item( const Item &i ) :usercomments(i.usercomments),
	systemcomments(i.systemcomments), msgid(i.msgid), msgstr(i.msgstr){}
    Item &operator=( const Item &i ) {
	usercomments = i.usercomments; systemcomments = i.systemcomments;
	msgid = i.msgid; msgstr = i.msgstr;
	return *this;
    }
    bool isNull() const { return usercomments.isNull()&&systemcomments.isNull()
			      &&msgstr.isNull() &&msgid.isNull(); }

    QString usercomments;
    QString systemcomments;
    QString msgid;
    QString msgstr;
};

enum Status { Equal, First, Second, FirstJunk, SecondJunk };

Item getItem( QTextStream &str, QString &s )
{
    Item i;
    while ( !str.atEnd() && s.isEmpty() ) {
	s = str.readLine().stripWhiteSpace();
    }
    while (  !str.atEnd() && !s.isEmpty() ) {
	if ( isSystemComment(s) ) {
	    i.systemcomments += s + "\n";
	    s = str.readLine().stripWhiteSpace();
	} else if ( isUserComment(s) ) {
	    i.usercomments += s + "\n";
	    s = str.readLine().stripWhiteSpace();
	} else if ( hasHandle( s, "msgid" ) ) {
	    QString r = s + "\n";
	    s = str.readLine().stripWhiteSpace();
	    while (  !str.atEnd() && hasHandle(s, "\"") ) {
		r += s;
		s = str.readLine().stripWhiteSpace();
		r += "\n";
	    }
	    i.msgid = r;
	} else if ( hasHandle( s, "msgstr" ) ) {
	    QString r = s + "\n";
	    s = str.readLine().stripWhiteSpace();
	    while ( hasHandle(s, "\"") ) {
		r += s;
		s = str.readLine().stripWhiteSpace();
		r += "\n";
	    }
	    i.msgstr = r;
	} else {
	    s = str.readLine().stripWhiteSpace();
	}
    }
    return i;
}

static int nMerge, nNew, nOld, nJunk;


void writecomment( QTextStream &str, const QString &s )
{
    if ( s.isEmpty() )
	return;
    int idx = 0;
    int len = s.length();
    while ( idx < len ) {
	int nl = s.find( '\n', idx );
	if ( nl < 0 ) {
	    qWarning( "writecomment: string lacks newline" );
	    str << "# " << s.mid( idx ) << '\n';
	    return;
	}
	str << "# " << s.mid( idx, nl-idx+1 );
	idx = nl+1;
    }
}

void writemerge( QTextStream &str, const Item &i1, const Item &i2 )
{
    nMerge++;
    if ( !i2.usercomments.isEmpty() )
	str << i2.usercomments;
    if ( !i1.systemcomments.isEmpty() )
	str << i1.systemcomments;
    str << i1.msgid;
    str << i2.msgstr;
    str << "\n";
}
void writenew( QTextStream &str, const Item &i1 )
{
    nNew++;
    if ( !i1.usercomments.isEmpty() )
	str << i1.usercomments;
    if ( !i1.systemcomments.isEmpty() )
	str << i1.systemcomments;
    str << i1.msgid;
    str << i1.msgstr;
    str << "\n";
}

void writeold( QTextStream &str, const Item &i2 )
{
    nOld++;
    writecomment( str, i2.usercomments );
    writecomment( str, i2.systemcomments );
    writecomment( str, i2.msgid );
    writecomment( str, i2.msgstr );
    str << "\n";
}

void writejunk( QTextStream &str, const Item &it )
{
    nJunk++;
    if ( !it.usercomments.isEmpty() )
	str << it.usercomments;
    writecomment( str,  it.systemcomments );
    writecomment( str,  it.msgid );
    writecomment( str,  it.msgstr );
    str << "\n";
}

Status compare( const Item &i1, const Item &i2 )
{
    if ( i1.isNull() && i2.isNull() )
	return Equal;
    if ( i1.isNull() )
	return Second;
    if ( i2.isNull() )
	return First;
    if ( i1.msgid.isEmpty() )
	return FirstJunk;
    if ( i2.msgid.isEmpty() )
	return SecondJunk;
    QString s1 = extractContents( i1.msgid );
    QString s2 = extractContents( i2.msgid );
    if ( !s1 && !s2 )
	return Equal;
    if ( !s1 )
	return First;
    if ( !s2 )
	return Second;
    int i = strcmp( s1.ascii(), s2.ascii() );
    if ( i < 0 )
	return First;
    if ( i == 0 )
	return Equal;
    // i > 0
	return Second;
}

void merge( const QString& newname, const QString& oldname,
	    const QString& resname )
{
    QFile f1(newname);
    if ( !f1.open( IO_ReadOnly) )
	return;

    QFile f2(oldname);
    if ( !f2.open( IO_ReadOnly) )
	return;

    QBuffer fout;
    fout.open(IO_WriteOnly);

    QTextStream in1( &f1 );
    QTextStream in2( &f2 );
    QTextStream out( &fout );

    QString buf1 = in1.readLine().stripWhiteSpace();
    QString buf2 = in2.readLine().stripWhiteSpace();

    Item i1 = getItem( in1, buf1 );
    Item i2 = getItem( in2, buf2 );
    while ( !i1.isNull() || !i2.isNull() ) {
	Status s = compare( i1, i2 );
	if ( s == Equal ) {
	    writemerge(out,i1,i2);
	    i1 = getItem( in1, buf1 );
	    i2 = getItem( in2, buf2 );
	} else if ( s == First ) {
	    writenew( out, i1 );
	    i1 = getItem( in1, buf1 );
	} else if ( s == FirstJunk ) {
	    //solitary comment
	    writejunk( out, i1 );
	    i1 = getItem( in1, buf1 );
	} else if ( s == Second ) {
	    writeold( out, i2 );
	    i2 = getItem( in2, buf2 );
	} else if ( s == SecondJunk ) {
	    //solitary comment
	    writejunk( out, i2 );
	    i2 = getItem( in2, buf2 );
	}
    }

    f1.close();
    f2.close();
    fout.close();
    QFile fileout(resname);
    if ( !fileout.open( IO_WriteOnly | IO_Translate ) )
	return;
    fileout.writeBlock(fout.buffer());
    fileout.close();
}


int main( int argc, char* argv[] )
{

    int orgfile = 1;
    int newfile = 2;

    if ( argc <= newfile || argc > 1 && argv[1][0] == '-') {
	printf("usage: %s  orgfile newfile [outfile]\n", argv[0]);
	exit(1);
    }

    merge( argv[newfile], argv[orgfile],
	   argc > newfile+1 ? argv[newfile+1] : argv[orgfile] );

    printf( "Merged %d entries, added %d new entries and removed %d entries\n",
	    nMerge, nNew, nOld );
    if ( nJunk > 0 )
	printf( "Found %d junk entries\n", nJunk );
    return 0;
}
