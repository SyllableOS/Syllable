//#include <time.h>
//#include <direct.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <process.h>
//#include <stdarg.h>
//#include <string.h>
//#include <stdio.h>

#include <atheos/types.h>
#include <atheos/string.h>
#include <atheos/v86.h>
#include <atheos/stdlib.h>
#include <posix/fcntl.h>
#include <macros.h>

#include "stage2.h"

int	isdigit( int c )
{
	return( c >= '0' && c <= '9' );
}

void ltoa( int Val, char* Str, uint32 Radix )
{
  int i,Len=0;

  if (!Val)
  {
    Str[0]='0';
    Str[1]=0;
    return;
  }

	if (Val<0)
  {
    Val=-Val;
    Len++;
    Str[0]='-';
  }

  for(i=Val;i;i/=Radix)
  {
    Len++;
  }

  Str[Len]=0;

  for(;Val;Val/=Radix)
  {
    uint8 Dig=Val%Radix;
    Dig+=(Dig<10) ? '0' : ('a'-10);
    Str[--Len]=Dig;
  }
}

void ultoa( uint32 Val, char* Str, uint32 Radix)
{
  int i,Len=0;

  if (!Val)
  {
    Str[0]='0';
    Str[1]=0;
    return;
  }
  for(i=Val;i;i/=Radix)
  {
    Len++;
  }

  Str[Len]=0;

  for(;Val;Val/=Radix)
  {
    uint8 Dig=Val%Radix;
    Dig+=(Dig<10) ? '0' : ('a'-10);
    Str[--Len]=Dig;
  }
}

long atol( const char* Str)
{
  int	Val=0;
  uint32 i=0,Exp=1;

  while(!(isdigit(Str[i]))) i++;
  while(isdigit(Str[i]))		i++;

  while(isdigit(Str[--i]))
  {
    Val+=(Str[i]-'0')*Exp;
    Exp*=10;
  }
  if (Str[i]=='-')
    return(-Val);
  else
    return(Val);
}


int sprintf( char* DstStr, const char* String,...)
{
#if 1
  va_list ap;

  uint8   CH;
  uint   ip=0,op=0,Len;
  va_start(ap,String);

  while(CH=String[ip++])
  {
    if (CH=='%')
    {
      uint8 NumStr[100];
      int  J=0;
/*      WORD  Size=2; */
      int  Size=4;
      int  Flag=0;
      int  Radix=8;
      int  Width=0;
      int  Prec=2;

      CH=String[ip++];
/*
      switch(CH)
      {
        case '-': Flag=1; CH=String[ip++]; break;
        case '+': Flag=2; CH=String[ip++]; break;
        case ' ': Flag=3; CH=String[ip++]; break;
      }
*/
      if (isdigit(CH))  // Read width argument
      {
        J=0;
        NumStr[J++]=CH;
        CH=String[ip++];

        while(isdigit(CH))
        {
          NumStr[J++]=CH;
          CH=String[ip++];
        }
        NumStr[J]=0;
        Width=atol(NumStr);
      }

      if (CH=='.')
      {
        J=0;
        while(isdigit(String[ip]))
        {
          CH=NumStr[J++]=String[ip++];
        }
        NumStr[J]=0;
        Prec=atol(NumStr);
      }


      switch(CH)  // Source size
      {
/*
        case 'F': Size=6; CH=String[ip++]; break;
        case 'N': Size=4; CH=String[ip++]; break;
        case 'h': Size=2; CH=String[ip++]; break;
*/
        case 'l': Size=4; CH=String[ip++]; break;
/*
        case 'L': Size=8; CH=String[ip++]; break;
*/
      }


      switch(CH)   // Set radix
      {
        case 'x': Radix=16; break;
/*
        case 'X': Radix=16; break;
        case 'N': Radix=16; break;
        case 'F': Radix=16; break;
*/
        case 'd': Radix=10; break;
/*
        case 'u': Radix=10; break;
        case 'i': Radix=10; break;
        case 'e': Radix=10; break;
        case 'f': Radix=10; break;
        case 'g': Radix=10; break;
        case 'o': Radix=8;  break;
*/
      }

      NumStr[0]=0;

      switch(CH)
      {
/*
        case 'N':
*/
        case 'x':
        case 'd':
/*
        case 'i':
        case 'o':
        case 'u':
*/
        {
          uint  Num;

          if (Size==2)
          {
            Num = va_arg(ap,int16);
            if (CH=='u')
              ultoa(Num,NumStr,Radix);
            else
              ltoa(Num,NumStr,Radix);
          }
          else
          {
            if (Size==4)
            {
              Num = va_arg(ap,uint);
              if (CH=='u')
                ultoa(Num,NumStr,Radix);
              else
                ltoa(Num,NumStr,Radix);
            }
          }

          Len=strlen(NumStr);

          if (Flag==1)  // left ustify
          {
            for(J=0;CH=NumStr[J++];DstStr[op++]=CH);
            if (Len<Width)
              for (J=0;J<Width-Len;J++) DstStr[op++]=' ';
          }
          else
          {
            if (Len<Width)
              for (J=0;J<Width-Len;J++) DstStr[op++]=' ';

            for(J=0;CH=NumStr[J++];DstStr[op++]=CH);
          }

          break;
        }

        case 's':
        {
          char*  Str;
          Str=va_arg(ap,char*);

          Len=strlen(Str);

          if (Flag==1)  // left ustify
          {
            while(Str[0]) DstStr[op++]=Str++[0];
            if (Len<Width)
              for (J=0;J<Width-Len;J++) DstStr[op++]=' ';
          }
          else
          {
            if (Len<Width)
              for (J=0;J<Width-Len;J++) DstStr[op++]=' ';
            while(Str[0]) DstStr[op++]=Str++[0];
          }

          while(Str[0]) DstStr[op++]=Str++[0];
          break;
        }

      }
      continue;
    }
    DstStr[op++]=CH;
  }
  DstStr[op]=0;
  va_end(ap);
#else
/*	strcpy( DstStr, String ); */
#endif

	return( 0 );
}






/******************************************************************************
 ******************************************************************************/

int printf( const char *fmt, ... )
{
#if 1
  char	zBuf[512];
  int		i;

/*	return( 0 ); */

  sprintf( zBuf, fmt, ((int*)&fmt)[1],
	   ((int*)&fmt)[2],
	   ((int*)&fmt)[3],
	   ((int*)&fmt)[4],
	   ((int*)&fmt)[5],
	   ((int*)&fmt)[6],
	   ((int*)&fmt)[7],
	   ((int*)&fmt)[8],
	   ((int*)&fmt)[9],
	   ((int*)&fmt)[10],
	   ((int*)&fmt)[11] );

  debug_write( zBuf, strlen( zBuf ) );
/*  
  for( i = 0 ;  '\0' != zBuf[i] ; ++i )
  {
    debug_write( &zBuf[i], 1 );
    if ( '\n' == zBuf[i] && '\r' != zBuf[ i + 1 ] ) {
      debug_write( "\r", 1 );
    }
  }
  */  
#endif
  return( 10 );
}

/******************************************************************************
 ******************************************************************************/

#if 0
void* memcpy(void *dst, const void *src, size_t size )
{
  char	*d = dst;
  const char	*s = src;

  while( size )
  {
    *d++ = *s++;
    size--;
  }
  return( dst );
}

/******************************************************************************
 ******************************************************************************/
void	*memset( void	*dst, int value, size_t size )
{
  char	*d = dst;

  while( size )
  {
    *d++ = (char) value;
    size--;
  }
  return( dst );
}

/******************************************************************************
 ******************************************************************************/

char*     strcpy( char *dst, const char *src )
{
  while( *dst++ = *src++ );
}

/******************************************************************************
 ******************************************************************************/

char*     strncpy( char *dst, const char *src, size_t len )
{
  while( (*dst++ = *src++) && len )
  {
    len--;
  }

  if ( len == 0 )
  {
    dst--;
    *dst = 0;
  }
}

/******************************************************************************
 ******************************************************************************/

size_t    strlen(const char	*str )
{
  size_t	len = 0;

  while(*str++)	len++;

  return( len );
}

int strncmp(const char * cs,const char * ct,size_t count)
{
  register signed char __res = 0;

  while (count) {
    if ((__res = *cs - *ct++) != 0 || !*cs++)
      break;
    count--;
  }

  return __res;
}
#endif
/******************************************************************************
 ******************************************************************************/

#if 0
int create( const char *path )
{
  struct RMREGS rm;

  memset( &rm, 0, sizeof(struct RMREGS) );

  rm.EAX = 0x3c00;
  rm.ECX = 0;			/* no attribs set	*/

  rm.SS	= (((uint32)RealPage ) >> 4 ) + 20;
  rm.DS	= ((uint32)RealPage) >> 4;
  strcpy( RealPage, path );

  realint( 0x21, &rm );

  return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
}
#endif

/******************************************************************************
 ******************************************************************************/

int open( const char *path, int access, ... )
{
  int cacc = 0;
  struct RMREGS rm;
/*
  if ( (access & O_CREAT) && ( !(access & O_RDONLY) ) )
  {
  return( create( path ) );
  }
  */
  if ( access & O_RDONLY		)	cacc |= 0x0000;
  if ( access & O_WRONLY		)	cacc |= 0x0001;
  if ( access & O_RDWR  		)	cacc |= 0x0002;

  memset( &rm, 0, sizeof(struct RMREGS) );

  rm.EAX = 0x3D00 | cacc;

  rm.SS = (((uint32)LIN_TO_PHYS( RealPage ) ) >> 4 ) + 20;
  rm.DS	= ((uint32)LIN_TO_PHYS( RealPage ) ) >> 4;
  strcpy( RealPage, path );

  realint( 0x21, &rm );

  return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
}

/******************************************************************************
 ******************************************************************************/

int close( int fn )
{
  struct	RMREGS rm;

  memset(&rm,0,sizeof(struct RMREGS));

  rm.EAX = 0x3E00;
  rm.EBX = fn;

  rm.SS = (((uint32)LIN_TO_PHYS( RealPage ) ) >> 4 ) + 20;
  realint( 0x21, &rm );

  return( 0 );
}

/******************************************************************************
 ******************************************************************************/

int realread( int handle, char *buffer, int size  )
{
  struct RMREGS rm;

  memset(&rm,0,sizeof(struct RMREGS));

  rm.EAX = 0x3F00;
  rm.EBX = handle;
  rm.ECX = size;

  rm.DS	 = ((uint32)LIN_TO_PHYS( RealPage ) ) >> 4;

  realint( 0x21, &rm );
  memcpy( buffer, RealPage, size );

  return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
}

/******************************************************************************
 ******************************************************************************/

int read( int fn, void *buffer, unsigned size )
{
  int	Bytes;
  int	BytesToRead,BytesRead;

  Bytes=0;

  while( Bytes < size )
  {
    if ( size - Bytes > 5536 )
      BytesToRead = 5536;
    else
      BytesToRead = size - Bytes;

    BytesRead = realread( fn, &((uint8*)buffer)[Bytes], BytesToRead );

    if ( BytesRead == -1 )
    {
      return(-1 );
      break;
    }
    Bytes += BytesRead;

    if ( BytesRead < BytesToRead )
    {
      break;
    }

  }	/* read loop	*/

  return( Bytes );
}

/******************************************************************************
 ******************************************************************************/


int realwrite( int handle, char *buffer, int size  )
{
  struct	RMREGS	rm;

  memset( &rm, 0, sizeof(struct RMREGS) );

  rm.EAX = 0x4000;
  rm.EBX = handle;
  rm.ECX = size;

  rm.DS	 = ((uint32)LIN_TO_PHYS( RealPage ) ) >> 4;

  memcpy( RealPage, buffer, size );
  realint( 0x21, &rm );

  return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
}

/******************************************************************************
 ******************************************************************************/

int write( int fn, const void *buffer, unsigned size )
{
#if 0
  int	Bytes = 0;
  int	BytesToWrite,BytesWritten;

  static	int	conout = -1;

  if ( fn < 2 )
  {
    if ( g_bDebug == FALSE )	return( size );

    if ( conout == -1 )
    {
      conout = open( "COM2", O_WRONLY );
    }
    fn = conout;
  }

  if ( fn != -1 )
  {

    while( Bytes < size )
    {
      if ( size - Bytes > 65536 )
	BytesToWrite = 65536;
      else
	BytesToWrite = size - Bytes;

      BytesWritten = realwrite( fn, &((uint8*)buffer)[Bytes], BytesToWrite );

      if ( BytesWritten == -1 )
      {
	return( -1 );
	break;
      }
      Bytes += BytesWritten;

      if ( BytesWritten < BytesToWrite )	break;
    }	/* write loop	*/

    return( Bytes );
  }
#endif
  return( -1 );
}

/******************************************************************************
 ******************************************************************************/

long	lseek( int fn, long disp, int mode )
{
  struct RMREGS rm;

  memset(&rm,0,sizeof(struct RMREGS));

  rm.EAX = 0x4200 | mode;
  rm.EBX = fn;
  rm.ECX = disp>>16;
  rm.EDX = disp & 0xffff;

  rm.SS = (((uint32)LIN_TO_PHYS( RealPage )) >> 4 ) + 20;
  realint( 0x21, &rm );

  return( ( rm.flags & 0x01 ) ? -1 : ((rm.EDX << 16 ) | (rm.EAX & 0xffff)) );
}
