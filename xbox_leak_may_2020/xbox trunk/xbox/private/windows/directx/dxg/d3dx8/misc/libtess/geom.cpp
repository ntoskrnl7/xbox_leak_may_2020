/*
** Copyright 1994, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Author: Eric Veach, July 1994.
*/

#include <assert.h>
#include "mesh.h"
#include "geom.h"

namespace D3DX
{


int __gl_vertLeq( GLUvertex *u, GLUvertex *v )
{
  /* Returns TRUE if u is lexicographically <= v. */

  return VertLeq( u, v );
}

GLdouble __gl_edgeEval( GLUvertex *u, GLUvertex *v, GLUvertex *w )
{
  /* Given three vertices u,v,w such that VertLeq(u,v) && VertLeq(v,w),
   * evaluates the t-coord of the edge uw at the s-coord of the vertex v.
   * Returns v->t - (uw)(v->s), ie. the signed distance from uw to v.
   * If uw is vertical (and thus passes thru v), the result is zero.
   *
   * The calculation is extremely accurate and stable, even when v
   * is very close to u or w.  In particular if we set v->t = 0 and
   * let r be the negated result (this evaluates (uw)(v->s)), then
   * r is guaranteed to satisfy MIN(u->t,w->t) <= r <= MAX(u->t,w->t).
   */
  GLdouble gapL, gapR;

  assert( VertLeq( u, v ) && VertLeq( v, w ));
  
  gapL = v->s - u->s;
  gapR = w->s - v->s;

  if( gapL + gapR > 0 ) {
    if( gapL < gapR ) {
      return (v->t - u->t) + (u->t - w->t) * (gapL / (gapL + gapR));
    } else {
      return (v->t - w->t) + (w->t - u->t) * (gapR / (gapL + gapR));
    }
  }
  /* vertical line */
  return 0;
}

GLdouble __gl_edgeSign( GLUvertex *u, GLUvertex *v, GLUvertex *w )
{
  /* Returns a number whose sign matches EdgeEval(u,v,w) but which
   * is cheaper to evaluate.  Returns > 0, == 0 , or < 0
   * as v is above, on, or below the edge uw.
   */
  GLdouble gapL, gapR;

  assert( VertLeq( u, v ) && VertLeq( v, w ));
  
  gapL = v->s - u->s;
  gapR = w->s - v->s;

  if( gapL + gapR > 0 ) {
    return (v->t - w->t) * gapL + (v->t - u->t) * gapR;
  }
  /* vertical line */
  return 0;
}


/***********************************************************************
 * Define versions of EdgeSign, EdgeEval with s and t transposed.
 */

GLdouble __gl_transEval( GLUvertex *u, GLUvertex *v, GLUvertex *w )
{
  /* Given three vertices u,v,w such that TransLeq(u,v) && TransLeq(v,w),
   * evaluates the t-coord of the edge uw at the s-coord of the vertex v.
   * Returns v->s - (uw)(v->t), ie. the signed distance from uw to v.
   * If uw is vertical (and thus passes thru v), the result is zero.
   *
   * The calculation is extremely accurate and stable, even when v
   * is very close to u or w.  In particular if we set v->s = 0 and
   * let r be the negated result (this evaluates (uw)(v->t)), then
   * r is guaranteed to satisfy MIN(u->s,w->s) <= r <= MAX(u->s,w->s).
   */
  GLdouble gapL, gapR;

  assert( TransLeq( u, v ) && TransLeq( v, w ));
  
  gapL = v->t - u->t;
  gapR = w->t - v->t;

  if( gapL + gapR > 0 ) {
    if( gapL < gapR ) {
      return (v->s - u->s) + (u->s - w->s) * (gapL / (gapL + gapR));
    } else {
      return (v->s - w->s) + (w->s - u->s) * (gapR / (gapL + gapR));
    }
  }
  /* vertical line */
  return 0;
}

GLdouble __gl_transSign( GLUvertex *u, GLUvertex *v, GLUvertex *w )
{
  /* Returns a number whose sign matches TransEval(u,v,w) but which
   * is cheaper to evaluate.  Returns > 0, == 0 , or < 0
   * as v is above, on, or below the edge uw.
   */
  GLdouble gapL, gapR;

  assert( TransLeq( u, v ) && TransLeq( v, w ));
  
  gapL = v->t - u->t;
  gapR = w->t - v->t;

  if( gapL + gapR > 0 ) {
    return (v->s - w->s) * gapL + (v->s - u->s) * gapR;
  }
  /* vertical line */
  return 0;
}


int __gl_vertCCW( GLUvertex *u, GLUvertex *v, GLUvertex *w )
{
  /* For almost-degenerate situations, the results are not reliable.
   * Unless the floating-point arithmetic can be performed without
   * rounding errors, *any* implementation will give incorrect results
   * on some degenerate inputs, so the client must have some way to
   * handle this situation.
   */
  return (u->s*(v->t - w->t) + v->s*(w->t - u->t) + w->s*(u->t - v->t)) >= 0;
}

/* Given parameters a,x,b,y returns the value (b*x+a*y)/(a+b),
 * or (x+y)/2 if a==b==0.  It requires that a,b >= 0, and enforces
 * this in the rare case that one argument is slightly negative.
 * The implementation is extremely stable numerically.
 * In particular it guarantees that the result r satisfies
 * MIN(x,y) <= r <= MAX(x,y), and the results are very accurate
 * even when a and b differ greatly in magnitude.
 */
#define RealInterpolate(a,x,b,y)			\
  (a = (a < 0) ? 0 : a, b = (b < 0) ? 0 : b,		\
  ((a <= b) ? ((b == 0) ? ((x+y) / 2)			\
                        : (x + (y-x) * (a/(a+b))))	\
            : (y + (x-y) * (b/(a+b)))))

#ifndef DEBUG
#define Interpolate(a,x,b,y)	RealInterpolate(a,x,b,y)
#else

/* Claim: the ONLY property the sweep algorithm relies on is that
 * MIN(x,y) <= r <= MAX(x,y).  This is a nasty way to test that.
 */
#include <stdlib.h>
extern int RandomInterpolate;

GLdouble Interpolate( GLdouble a, GLdouble x, GLdouble b, GLdouble y)
{
#ifndef NT
printf("*********************%d\n",RandomInterpolate);
#endif
  if( RandomInterpolate ) {
    a = 1.2 * drand48() - 0.1;
    a = (a < 0) ? 0 : ((a > 1) ? 1 : a);
    b = 1.0 - a;
  }
  return RealInterpolate(a,x,b,y);
}

#endif

#define Swap(a,b)	if (1) { GLUvertex *t = a; a = b; b = t; } else

void __gl_edgeIntersect( GLUvertex *o1, GLUvertex *d1,
			 GLUvertex *o2, GLUvertex *d2,
			 GLUvertex *v )
/* Given edges (o1,d1) and (o2,d2), compute their point of intersection.
 * The computed point is guaranteed to lie in the intersection of the
 * bounding rectangles defined by each edge.
 */
{
  GLdouble z1, z2;

  /* This is certainly not the most efficient way to find the intersection
   * of two line segments, but it is very numerically stable.
   *
   * Strategy: find the two middle vertices in the VertLeq ordering,
   * and interpolate the intersection s-value from these.  Then repeat
   * using the TransLeq ordering to find the intersection t-value.
   */

  if( ! VertLeq( o1, d1 )) { Swap( o1, d1 ); }
  if( ! VertLeq( o2, d2 )) { Swap( o2, d2 ); }
  if( ! VertLeq( o1, o2 )) { Swap( o1, o2 ); Swap( d1, d2 ); }

  if( ! VertLeq( o2, d1 )) {
    /* Technically, no intersection -- do our best */
    v->s = (o2->s + d1->s) / 2;
  } else if( VertLeq( d1, d2 )) {
    /* Interpolate between o2 and d1 */
    z1 = EdgeEval( o1, o2, d1 );
    z2 = EdgeEval( o2, d1, d2 );
    if( z1+z2 < 0 ) { z1 = -z1; z2 = -z2; }
    v->s = Interpolate( z1, o2->s, z2, d1->s );
  } else {
    /* Interpolate between o2 and d2 */
    z1 = EdgeSign( o1, o2, d1 );
    z2 = -EdgeSign( o1, d2, d1 );
    if( z1+z2 < 0 ) { z1 = -z1; z2 = -z2; }
    v->s = Interpolate( z1, o2->s, z2, d2->s );
  }

  /* Now repeat the process for t */

  if( ! TransLeq( o1, d1 )) { Swap( o1, d1 ); }
  if( ! TransLeq( o2, d2 )) { Swap( o2, d2 ); }
  if( ! TransLeq( o1, o2 )) { Swap( o1, o2 ); Swap( d1, d2 ); }

  if( ! TransLeq( o2, d1 )) {
    /* Technically, no intersection -- do our best */
    v->t = (o2->t + d1->t) / 2;
  } else if( TransLeq( d1, d2 )) {
    /* Interpolate between o2 and d1 */
    z1 = TransEval( o1, o2, d1 );
    z2 = TransEval( o2, d1, d2 );
    if( z1+z2 < 0 ) { z1 = -z1; z2 = -z2; }
    v->t = Interpolate( z1, o2->t, z2, d1->t );
  } else {
    /* Interpolate between o2 and d2 */
    z1 = TransSign( o1, o2, d1 );
    z2 = -TransSign( o1, d2, d1 );
    if( z1+z2 < 0 ) { z1 = -z1; z2 = -z2; }
    v->t = Interpolate( z1, o2->t, z2, d2->t );
  }
}

}