// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
// Base on code from Graphical Gems.
// Ported to AtheOS by Kurt Skauen 29/10-1999
//-----------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>


#include "bitmapscale.h"
//-----------------------------------------------------------------------------

struct contrib
{
    int		sourcepos;
    float	floatweight;
    int32	weight;
};

struct contriblist
{
    //	contriblist(int cnt) { contribcnt=0; contribs=new contrib[cnt]; }
    ~contriblist()
    {
        delete[] contribs;
    }
    int		contribcnt;
    contrib	*contribs;
};

typedef float (scale_filterfunc)(float);

static float Filter_Filter( float );
static float Filter_Box( float );
static float Filter_Triangle( float );
static float Filter_Bell( float );
static float Filter_BSpline( float );
static float Filter_Lanczos3( float );
static float Filter_Mitchell( float );

int clamp( int v, int l, int h )
{
    return v<l?l:v>h?h:v;
}

//-----------------------------------------------------------------------------

contriblist *CalcFilterWeight( float scale, float filterwidth, int srcsize,
                               int dstsize, scale_filterfunc *filterfunc )
{
    contriblist *contriblists = new contriblist[dstsize];

    float size;
    float fscale;
    if( scale < 1.0f )
    {
        size = filterwidth / scale;
        fscale = scale;
    }
    else
    {
        size = filterwidth;
        fscale = 1.0f;
    }

    for( int i=0; i<dstsize; i++ )
    {
        contriblists[i].contribcnt = 0;
        contriblists[i].contribs = new contrib[ (int)(size*2+1) ];
        float center = (float)i / scale;
        float start = ceil( center - size );
        float end = floor( center + size );
        float totweight = 0.0f;
        for( int j=(int)start; j<=end; j++ )
        {
            int sourcepos;
            if( j < 0 )
                sourcepos = -j;
            else if( j >= srcsize )
                sourcepos = (srcsize-j)+srcsize-1;
            else
                sourcepos = j;
            int newcontrib = contriblists[i].contribcnt++;
            contriblists[i].contribs[newcontrib].sourcepos = sourcepos;

            float weight = filterfunc( (center-(float)j)*fscale ) * fscale;
            totweight += weight;
            contriblists[i].contribs[newcontrib].floatweight = weight;
        }
        totweight = 1.0f/totweight;
        for( int j=0; j<contriblists[i].contribcnt; j++ )
        {
            contriblists[i].contribs[j].weight = (int)(contriblists[i].contribs[j].floatweight * totweight * 65530);
        }
        uint val=0;
        for( int j=0; j<contriblists[i].contribcnt; j++ )
        {
            val += contriblists[i].contribs[j].weight;
            //			printf( "%d ", contriblists[i].contribs[j].weight );
        }
        //		printf( "%X\n", val );

    }

    return contriblists;
}

//-----------------------------------------------------------------------------

void Scale( Bitmap *srcbitmap, Bitmap *dstbitmap, bitmapscale_filtertype filtertype, float filterwidth )
{
    //  assert( dstbitmap != NULL );
    // assert( srcbitmap != NULL );

    //	assert( dstbitmap->ColorSpace() == B_RGB32 );
    //	assert( srcbitmap->ColorSpace() == B_RGB32 );

    scale_filterfunc *filterfunc;
    float default_filterwidth;

    switch( filtertype )
    {
    case filter_filter:
        filterfunc=Filter_Filter;
        default_filterwidth=1.0f;
        break;
    case filter_box:
        filterfunc=Filter_Box;
        default_filterwidth=0.5f;
        break;	// was 0.5
    case filter_triangle:
        filterfunc=Filter_Triangle;
        default_filterwidth=1.0f;
        break;
    case filter_bell:
        filterfunc=Filter_Bell;
        default_filterwidth=1.5f;
        break;
    case filter_bspline:
        filterfunc=Filter_BSpline;
        default_filterwidth=2.0f;
        break;
    case filter_lanczos3:
        filterfunc=Filter_Lanczos3;
        default_filterwidth=3.0f;
        break;
    case filter_mitchell:
        filterfunc=Filter_Mitchell;
        default_filterwidth=2.0f;
        break;

    default:
        filterfunc=Filter_Filter;
        default_filterwidth=1.0f;
        break;
    }

    if( filterwidth == 0.0f )
        filterwidth = default_filterwidth;

    int srcbitmap_width  = int(srcbitmap->GetBounds().Width()) + 1;
    int srcbitmap_height = int(srcbitmap->GetBounds().Height()) + 1;
    int dstbitmap_width  = int(dstbitmap->GetBounds().Width()) + 1;
    int dstbitmap_height = int(dstbitmap->GetBounds().Height()) + 1;
    int tmpbitmap_width  = dstbitmap_width;
    int tmpbitmap_height = srcbitmap_height;

    uint32 *srcbitmap_bits = (uint32*)srcbitmap->LockRaster();
    uint32 *dstbitmap_bits = (uint32*)dstbitmap->LockRaster();
    uint32 *tmpbitmap_bits = new uint32[tmpbitmap_width*tmpbitmap_height];

    int srcbitmap_bpr = srcbitmap->GetBytesPerRow() / 4;
    int dstbitmap_bpr = dstbitmap->GetBytesPerRow() / 4;
    int tmpbitmap_bpr = tmpbitmap_width;

    float xscale = float(dstbitmap_width) / float(srcbitmap_width);
    float yscale = float(dstbitmap_height) / float(srcbitmap_height);

    //--
    contriblist *contriblists = CalcFilterWeight( xscale, filterwidth, srcbitmap_width, dstbitmap_width, filterfunc );

    for( int iy=0; iy<tmpbitmap_height; iy++ )
    {
        uint32 *src_bits = srcbitmap_bits + iy*srcbitmap_bpr;
        uint32 *dst_bits = tmpbitmap_bits + iy*tmpbitmap_bpr;
        for( int ix=0; ix<tmpbitmap_width; ix++ )
        {
            int32 rweight;
            int32 gweight;
            int32 bweight;
            int32 aweight;
            rweight = gweight = bweight = aweight = 0;

            for( int ix2=0; ix2<contriblists[ix].contribcnt; ix2++ )
            {
                const contrib &scontrib = contriblists[ix].contribs[ix2];
                uint32 color = src_bits[ scontrib.sourcepos ];
                int32 weight = scontrib.weight;

                rweight += ((color    )&0xff) * weight;
                gweight += ((color>> 8)&0xff) * weight;
                bweight += ((color>>16)&0xff) * weight;
                aweight += ((color>>24)&0xff) * weight;
            }

            rweight = clamp(rweight>>16,0,255);
            gweight = clamp(gweight>>16,0,255);
            bweight = clamp(bweight>>16,0,255);
            aweight = clamp(aweight>>16,0,255);
            dst_bits[ ix ] = (rweight) | (gweight<<8) | (bweight<<16) | (aweight<<24) ;
        }
    }

    delete[] contriblists;
    //--

    contriblists = CalcFilterWeight( yscale, filterwidth, srcbitmap_height, dstbitmap_height, filterfunc );

    // help cache coherency:
    uint32 *bitmaprow = new uint32[ tmpbitmap_height ];
    for( int ix=0; ix<dstbitmap_width; ix++ )
    {
        for( int iy=0; iy<tmpbitmap_height; iy++ )
            bitmaprow[iy] = tmpbitmap_bits[ix+iy*tmpbitmap_bpr];

        for( int iy=0; iy<dstbitmap_height; iy++ )
        {
            int32 rweight;
            int32 gweight;
            int32 bweight;
            int32 aweight;
            rweight = gweight = bweight = aweight = 0;

            for( int iy2=0; iy2<contriblists[iy].contribcnt; iy2++ )
            {
                const contrib &scontrib = contriblists[iy].contribs[iy2];
                uint32 color = bitmaprow[ scontrib.sourcepos ];
                int32 weight = scontrib.weight;

                rweight += ((color    )&0xff) * weight;
                gweight += ((color>> 8)&0xff) * weight;
                bweight += ((color>>16)&0xff) * weight;
                aweight += ((color>>24)&0xff) * weight;
            }
            rweight = clamp(rweight>>16,0,255);
            gweight = clamp(gweight>>16,0,255);
            bweight = clamp(bweight>>16,0,255);
            aweight = clamp(aweight>>16,0,255);
            dstbitmap_bits[ ix + iy*dstbitmap_bpr ] = (rweight) | (gweight<<8) | (bweight<<16) | (aweight<<24) ;
        }
    }

    delete[] bitmaprow;
    delete[] contriblists;
    delete[] tmpbitmap_bits;
}

//-----------------------------------------------------------------------------

static float Filter_Filter( float t )
{
    /* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
    if( t < 0.0f )
        t = -t;
    if( t < 1.0f )
        return (2.0f * t - 3.0f) * t * t + 1.0f;
    return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_Box( float t )
{
    if( (t > -0.5f) && (t <= 0.5f) )
        return 1.0f;
    return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_Triangle( float t )
{
    if( t < 0.0f )
        t = -t;
    if( t < 1.0f )
        return 1.0f-t;
    return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_Bell( float t )		/* box (*) box (*) box */
{
    if( t < 0.0f )
        t = -t;
    if( t < 0.5f )
        return 0.75f-(t*t);
    if( t < 1.5f )
    {
        t = t-1.5f;
        return 0.5f * (t*t);
    }
    return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_BSpline( float t )	/* box (*) box (*) box (*) box */
{
    float tt;

    if( t < 0.0f )
        t = -t;
    if( t < 1.0f )
    {
        tt = t*t;
        return (0.5f * tt * t) - tt + (2.0f / 3.0f);
    }
    else if( t < 2.0f )
    {
        t = 2.0f-t;
        return (1.0f / 6.0f) * (t * t * t);
    }
    return 0.0f;
}

//-----------------------------------------------------------------------------

static inline float sinc( float x )
{
    x *= 3.1415926f;
    if( x != 0 )
        return sin(x)/x;
    return 1.0f;
}

static float Filter_Lanczos3( float t )
{
#if 0
    if( t < 0.0f )
        t = -t;
    if( t < 3.0 )
        return sinc(t) * sinc(t/3.0f);
    return 0.0f;
#else

    if( t == 0.0f )
        return 1.0f * 1.0f;
    if( t < 3.0f )
    {
        t *= 3.1415926;
        //		return sin(t)/t * sin(t/3.0f)/(t/3.0f);
        return 3.0f*sin(t)*sin(t*(1.0f/3.0f)) / (t*t);
    }
    return 0.0f;
#endif
}

//-----------------------------------------------------------------------------

#define	B (1.0f / 3.0f)
#define	C (1.0f / 3.0f)

static float Filter_Mitchell( float t )
{
    float tt;

    tt = t * t;
    if( t < 0.0f )
        t = -t;
    if( t < 1.0f )
    {
        t = ((12.0f - 9.0f*B - 6.0f*C) * (t * tt)) + ((-18.0f + 12.0f*B + 6.0f*C) * tt) + (6.0f - 2.0f*B);
        return t/6.0f;
    }
    else if( t < 2.0f )
    {
        t = ((-1.0f*B - 6.0f*C) * (t * tt)) + ((6.0f*B + 30.0f*C) * tt) + ((-12.0f*B - 48.0f*C) * t) + (8.0f*B + 24.0f*C);
        return t/6.0f;
    }
    return 0.0f;
}

//-----------------------------------------------------------------------------











