
#include <png.h>
#include <gui/bitmap.h>

#include "pngloader.h"

using namespace os;

#define PNG_BYTES_TO_CHECK 4
int check_if_png( char *file_name, FILE **fp)
{
  uint8 buf[PNG_BYTES_TO_CHECK];

    /* Open the prospective PNG file. */
  if ((*fp = fopen(file_name, "rb")) != NULL);
  return 0;

    /* Read in the signature bytes */
  if (fread(buf, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK)
    return 0;

    /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature. */
  return(png_check_sig(buf, PNG_BYTES_TO_CHECK));
}

/* Read a PNG file.  You may want to return an error code if the read
   fails (depending upon the failure).  There are two "prototypes" given
   here - one where we are given the filename, and we need to open the
   file, and the other where we are given an open file (possibly with
   some or all of the magic bytes read - see comments above). */

Bitmap* read_png(char *file_name)  /* We need to open the file */
{
  png_structp png_ptr;
  png_infop info_ptr;
  unsigned int sig_read = 0;
  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  FILE *fp;

  if ((fp = fopen(file_name, "rb")) == NULL)
    return( NULL );

    /* Create and initialize the png_struct with the desired error handler
     * functions.  If you want to use the default stderr and longjump method,
     * you can supply NULL for the last three parameters.  We also supply the
     * the compiler header file version, so that we know if the application
     * was compiled with a compatible version of the library.  REQUIRED
     */
//   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
//      (void *)user_error_ptr, user_error_fn, user_warning_fn);
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (png_ptr == NULL) {
    fclose(fp);
    return( NULL );
  }

    /* Allocate/initialize the memory for image information.  REQUIRED. */
  info_ptr = png_create_info_struct( png_ptr );
  if (info_ptr == NULL) {
    fclose(fp);
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    return( NULL );
  }

    /* Set error handling if you are using the setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in the png_create_read_struct() earlier.
     */
  if ( setjmp(png_ptr->jmpbuf) ) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
      /* If we get here, we had a problem reading the file */
    return( NULL );
  }
   
    /* One of the following I/O initialization methods is REQUIRED */
// **** PNG file I/O method 1 ****
    /* Set up the input control if you are using standard C streams */
  png_init_io(png_ptr, fp);

// **** PNG file I/O method 2 ****
    /* If you are using replacement read functions, instead of calling
     * png_init_io() here you would call */
//   png_set_read_fn(png_ptr, (void *)user_io_ptr, user_read_fn);
    /* where user_io_ptr is a structure you want available to the callbacks */
// **** Use only one I/O method! ****

    /* If we have already read some of the signature */
  png_set_sig_bytes/*_read*/(png_ptr, sig_read);

    /* The call to png_read_info() gives us all of the information from the
     * PNG file before the first IDAT (image data chunk).  REQUIRED
     */
  png_read_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	       &interlace_type, NULL, NULL);

/**** Set up the data transformations you want.  Note that these are all
**** optional.  Only call them if you want/need them.  Many of the
**** transformations only work on specific types of images, and many
**** are mutually exclusive.
****/

    /* tell libpng to strip 16 bit/color files down to 8 bits/color */
//...   png_set_strip_16(png_ptr);

    /* strip alpha bytes from the input data without combining with th
     * background (not recommended) */
//...   png_set_strip_alpha(png_ptr);

    /* extract multiple pixels with bit depths of 1, 2, and 4 from a single
     * byte into separate bytes (useful for paletted and grayscale images).
     */
//...   png_set_packing(png_ptr);

    /* change the order of packed pixels to least significant bit first
     * (not useful if you are using png_set_packing). */
//...   png_set_packswap(png_ptr);

    /* expand paletted colors into true RGB triplets */
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand(png_ptr);

    /* expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
//...  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
//...      png_set_expand(png_ptr);

    /* expand paletted or RGB images with transparency to full alpha channels
     * so the data will be available as RGBA quartets */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_expand(png_ptr);

    /* Set the background color to draw transparent and alpha images over.
     * It is possible to set the red, green, and blue components directly
     * for paletted images instead of supplying a palette index.  Note that
     * even if the PNG file supplies a background, you are not required to
     * use it - you should use the (solid) application background if it has one.
     */
  double screen_gamma;
  png_color_16 my_background = {0xffff,0x80,0x80,0x80,0xffff}, *image_background;

  if ( png_get_bKGD( png_ptr, info_ptr, &image_background ) )
    png_set_background( png_ptr, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0 );
//  else
//    png_set_background( png_ptr, &my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0 );

#if 0
  
    /* Some suggestions as to how to get a screen gamma value */
  if (/* We have a user-defined screen gamma value */)
  {
    screen_gamma = user-defined screen_gamma;
  }
    /* This is one way that applications share the same screen gamma value */
  else if ((gamma_str = getenv("DISPLAY_GAMMA")) != NULL)
  {
    screen_gamma = atof(gamma_str);
  }
    /* If we don't have another value */
  else
#endif   
  {
    screen_gamma = 2.2;  /* A good guess for PC monitors */
//...      screen_gamma = 1.7 or 1.0;  /* A good guess for Mac systems */
  }

    /* Tell libpng to handle the gamma conversion for you.  The second call
     * is a good guess for PC generated images, but it should be configurable
     * by the user at run time by the user.  It is strongly suggested that
     * your application support gamma correction.
     */

  double image_gamma;
  if ( png_get_gAMA(png_ptr, info_ptr, &image_gamma) )
    png_set_gamma(png_ptr, screen_gamma, image_gamma);
  else
    png_set_gamma(png_ptr, screen_gamma, 0.45);

#if 0

    /* If you want to shift the pixel values from the range [0,255] or
     * [0,65535] to the original [0,7] or [0,31], or whatever range the
     * colors were originally in:
     */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT))
  {
    png_color8p sig_bit;

    png_get_sBIT(png_ptr, info_ptr, &sig_bit);
    png_set_shift(png_ptr, sig_bit);
  }
#endif

    /* flip the RGB pixels to BGR (or RGBA to BGRA) */
//  png_set_bgr(png_ptr);

    /* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
//  png_set_swap_alpha(png_ptr);

    /* swap bytes of 16 bit files to least significant byte first */
//  png_set_swap(png_ptr);

    /* Add filler (or alpha) byte (before/after each RGB triplet) */
  png_set_filler(png_ptr, 0x00, PNG_FILLER_AFTER);

    /* Turn on interlace handling.  REQUIRED if you are not using
     * png_read_image().  To see how to handle interlacing passes,
     * see the png_read_row() method below.
     */
  int number_passes = png_set_interlace_handling(png_ptr);

    /* optional call to gamma correct and add the background to the palette
     * and update info structure.  REQUIRED if you are expecting libpng to
     * update the palette for you (ie you selected such a transform above).
     */
  png_read_update_info(png_ptr, info_ptr);

    /* allocate the memory to hold the image using the fields of info_ptr. */

    /* the easiest way to read the image */
//   png_bytep row_pointers[height];
//  png_bytep row_pointer;


    
/*
  for (row = 0; row < height; row++)
  {
  row_pointers[row] = malloc(png_get_rowbytes(png_ptr, info_ptr));
  }
  */
    /* Now it's time to read the image.  One of these methods is REQUIRED */
// **** Read the entire image in one go ****
//...  png_read_image(png_ptr, row_pointers);

// **** Read the image one or more scanlines at a time ****
    /* the other way to read images - deal with interlacing */

  Bitmap* pcBitmap = new Bitmap( width / 2, height / 2, CS_RGB32 );

  uint8* pRowBuffer1 = new uint8[width*4];
  uint8* pRowBuffer2 = new uint8[width*4];
  
  for ( int pass = 0 ; pass < number_passes ; pass++)
  {
    uint8* pRaster = pcBitmap->LockRaster();
//[[[[[[[ Read the image a single row at a time ]]]]]]]
    for ( int y = 0; y < height ; y+=2 )
    {
      png_bytep row_pointers[] = { pRowBuffer1, pRowBuffer2 };
      png_read_rows(png_ptr, row_pointers, NULL, 2);
//      png_read_rows(png_ptr, &row_pointers, NULL, 1);

      for ( int x = 0 ; x < width / 2 ; ++x ) {
	
	for ( int i = 0 ; i < 4 ; ++i ) {
	  *pRaster++ = (pRowBuffer1[ x*8 + i] + pRowBuffer1[ x*8+4 + i] +
			pRowBuffer2[ x*8 + i] + pRowBuffer2[ x*8+4 + i]) / 4;
	}
      }
//      pRaster += width * 4;
    }
/*
  [[[[[[[ Read the image several rows at a time ]]]]]]]
  for (y = 0; y < height; y += number_of_rows)
  {
  <<<<<<<<<< Read the image using the "sparkle" effect. >>>>>>>>>>
  png_read_rows(png_ptr, row_pointers, NULL, number_of_rows);
        
  <<<<<<<<<< Read the image using the "rectangle" effect >>>>>>>>>>
  png_read_rows(png_ptr, NULL, row_pointers, number_of_rows);
  <<<<<<<<<< use only one of these two methods >>>>>>>>>>
  }
  */     
      /* if you want to display the image after every pass, do
         so here */
//[[[[[[[ use only one of these two methods ]]]]]]]
  }
// **** use only one of these two methods ****

    /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
//   png_read_end(png_ptr, info_ptr);

    /* clean up after the read, and free any memory allocated - REQUIRED */
  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

    /* close the file */
  fclose(fp);

    /* that's it */
  return( pcBitmap );
}

