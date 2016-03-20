/*********************************************************************
 * klt_util.c
 *********************************************************************/

/* Standard includes */
#include <assert.h>
#include <stdlib.h>  /* malloc() */
#include <math.h>		/* fabs() */

/* Our includes */
#include "base.h"
#include "error.h"
#include "pnmio.h"
#include "klt.h"
#include "klt_util.h"


/*********************************************************************/

float _KLTComputeSmoothSigma(
  KLT_TrackingContext tc)
{
  return (tc->smooth_sigma_fact * max(tc->window_width, tc->window_height));
}


/*********************************************************************
 * _KLTCreateFloatImage
 */

_KLT_FloatImage _KLTCreateFloatImage(
  int ncols,
  int nrows)
{
  _KLT_FloatImage floatimg;
  int nbytes = sizeof(_KLT_FloatImageRec) +
    ncols * nrows * sizeof(float);

  floatimg = (_KLT_FloatImage)  malloc(nbytes);
  if (floatimg == NULL)
    KLTError("(_KLTCreateFloatImage)  Out of memory");
  floatimg->ncols = ncols;
  floatimg->nrows = nrows;
  floatimg->data = (float *)  (floatimg + 1);

  return(floatimg);
}


/*********************************************************************
 * _KLTFreeFloatImage
 */

void _KLTFreeFloatImage(
  _KLT_FloatImage floatimg)
{
  free(floatimg);
}


