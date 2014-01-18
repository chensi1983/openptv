/****************************************************************************

Routine:            multimed.c

Author/Copyright:       Hans-Gerd Maas

Address:            Institute of Geodesy and Photogrammetry
                ETH - Hoenggerberg
                    CH - 8093 Zurich

Creation Date:          20.4.88
    
Description:            
1 point, exterior orientation, multimedia parameters    ==> X,Y=shifts
          (special case 3 media, medium 2 = plane parallel)
                            
Routines contained:     -

****************************************************************************/

#include "multimed.h"


void  multimed_nlay (Exterior ex, Exterior ex_o, mm_np mm, \
double X, double Y, double Z, double *Xq, double *Yq, int cam){
  
  /* Beat L�thi, Nov 2007 comment actually only Xq is affected since all Y and Yq are always zero */
  int       i, it=0;
  double    beta1, beta2[32], beta3, r, rbeta, rdiff, rq, mmf;
  mmlut     *mmLUT;
  
  /* interpolation in mmLUT, if selected (requires some global variables) */
  if (mm.lut){         
      mmf = get_mmf_from_mmLUT (cam, X,Y,Z, (mmlut *)mmLUT);
      
      if (mmf > 0)
      {
          *Xq = ex.x0 + (X-ex.x0) * mmf;
          *Yq = ex.y0 + (Y-ex.y0) * mmf;
          return;
       }
    }
  
  // iterative procedure (if mmLUT does not exist or has no entry) 
  r = sqrt ((X-ex.x0)*(X-ex.x0)+(Y-ex.y0)*(Y-ex.y0));
  rq = r;
  
  do
    {
      beta1 = atan (rq/(ex.z0-Z));
      for (i=0; i<mm.nlay; i++) beta2[i] = asin (sin(beta1) * mm.n1/mm.n2[i]);
      beta3 = asin (sin(beta1) * mm.n1/mm.n3);
      
      rbeta = (ex.z0-mm.d[0]) * tan(beta1) - Z * tan(beta3);
      for (i=0; i<mm.nlay; i++) rbeta += (mm.d[i] * tan(beta2[i]));
      rdiff = r - rbeta;
      rq += rdiff;
      it++;
    }
  while (((rdiff > 0.001) || (rdiff < -0.001))  &&  it < 40);
  
  if (it >= 40)
    {
      *Xq = X; *Yq = Y;
      printf("Multimed_nlay stopped after 40. Iteration\n");   return;
    }
    
  if (r != 0)
    {
      *Xq = ex.x0 + (X-ex.x0) * rq/r;
      *Yq = ex.y0 + (Y-ex.y0) * rq/r;
    }
  else
    {
      *Xq = X;
      *Yq = Y;
    }
    
}

void back_trans_Point_back(double X_t, double Y_t, double Z_t,mm_np mm, Glass G, \
double cross_p[], double cross_c[], double *X, double *Y, double *Z){

    double nVe,nGl;
    nGl=sqrt(pow(G.vec_x,2.)+pow(G.vec_y,2.)+pow(G.vec_z,2.));
    nVe=sqrt( pow(cross_p[0]-(cross_c[0]-mm.d[0]*G.vec_x/nGl),2.)
             +pow(cross_p[1]-(cross_c[1]-mm.d[0]*G.vec_y/nGl),2.)
             +pow(cross_p[2]-(cross_c[2]-mm.d[0]*G.vec_z/nGl),2.));
    

    *X=cross_c[0]+X_t*(cross_p[0]-(cross_c[0]-mm.d[0]*G.vec_x/nGl))/nVe+Z_t*G.vec_x/nGl;
    *Y=cross_c[1]+X_t*(cross_p[1]-(cross_c[1]-mm.d[0]*G.vec_y/nGl))/nVe+Z_t*G.vec_y/nGl;
    *Z=cross_c[2]+X_t*(cross_p[2]-(cross_c[2]-mm.d[0]*G.vec_z/nGl))/nVe+Z_t*G.vec_z/nGl;
}



void trans_Cam_Point_back(Exterior ex
						, mm_np mm
						, Glass gl
						, double X
						, double Y
						, double Z
						, Exterior *ex_t
						, double *X_t
						, double *Y_t
						, double *Z_t
						, double cross_p[3]
						, double cross_c[3]){

  //--Beat L�thi June 07: I change the stuff to a system perpendicular to the interface
  double dummy;
  double nGl;
  
  nGl = sqrt(gl.vec_x * gl.vec_x + gl.vec_y * gl.vec_y + gl.vec_z * gl.vec_z);
  
  dummy = ex.x0 * gl.vec_x / nGl + ex.y0 * gl.vec_y / nGl + ex.z0 * gl.vec_z / nGl \
  - nGl - mm.d[0];
  cross_c[0] = ex.x0 - dummy * gl.vec_x / nGl;
  cross_c[1] = ex.y0 - dummy * gl.vec_y / nGl;
  cross_c[2] = ex.z0 - dummy * gl.vec_z / nGl;
  ex_t->x0 = 0.;
  ex_t->y0 = 0.;
  ex_t->z0 = dummy;

  dummy = nGl - X * gl.vec_x / nGl + Y * gl.vec_y / nGl + Z * gl.vec_z / nGl;
  cross_p[0] = X + dummy * gl.vec_x / nGl;
  cross_p[1] = Y + dummy * gl.vec_y / nGl;
  cross_p[2] = Z + dummy * gl.vec_z / nGl;
  *Z_t = -dummy - mm.d[0];
  dummy = sqrt(pow(cross_p[0] - (cross_c[0] - mm.d[0] * gl.vec_x / nGl ), 2.)
             + pow(cross_p[1] - (cross_c[1] - mm.d[0] * gl.vec_y / nGl ), 2.)
             + pow(cross_p[2] - (cross_c[2] - mm.d[0] * gl.vec_z / nGl), 2.));
  *X_t = dummy;
  *Y_t = 0;
      
}

void trans_Cam_Point(Exterior ex, mm_np mm, Glass gl, double X, double Y, double Z, \
Exterior *ex_t, double *X_t, double *Y_t, double *Z_t, double cross_p[3], double cross_c[3]){

  //--Beat L�thi June 07: I change the stuff to a system perpendicular to the interface
  double dist_cam_glas,dist_point_glas,dist_o_glas; //glas inside at water 
  
  dist_o_glas=sqrt(gl.vec_x*gl.vec_x+gl.vec_y*gl.vec_y+gl.vec_z*gl.vec_z);
  dist_cam_glas   = ex.x0*gl.vec_x/dist_o_glas+ex.y0*gl.vec_y/dist_o_glas+ex.z0*gl.vec_z/dist_o_glas-dist_o_glas-mm.d[0];
  dist_point_glas = X    *gl.vec_x/dist_o_glas+Y    *gl.vec_y/dist_o_glas+Z    *gl.vec_z/dist_o_glas-dist_o_glas; 

  cross_c[0]=ex.x0-dist_cam_glas*gl.vec_x/dist_o_glas;
  cross_c[1]=ex.y0-dist_cam_glas*gl.vec_y/dist_o_glas;
  cross_c[2]=ex.z0-dist_cam_glas*gl.vec_z/dist_o_glas;
  cross_p[0]=X    -dist_point_glas*gl.vec_x/dist_o_glas;
  cross_p[1]=Y    -dist_point_glas*gl.vec_y/dist_o_glas;
  cross_p[2]=Z    -dist_point_glas*gl.vec_z/dist_o_glas;

  ex_t->x0=0.;
  ex_t->y0=0.;
  ex_t->z0=dist_cam_glas+mm.d[0];

  *X_t=sqrt( pow(cross_p[0]-(cross_c[0]-mm.d[0]*gl.vec_x/dist_o_glas),2.)
            +pow(cross_p[1]-(cross_c[1]-mm.d[0]*gl.vec_y/dist_o_glas),2.)
            +pow(cross_p[2]-(cross_c[2]-mm.d[0]*gl.vec_z/dist_o_glas),2.));
  *Y_t=0;
  *Z_t=dist_point_glas;
      
}

void back_trans_Point(double X_t, double Y_t, double Z_t, mm_np mm, Glass G, \
double cross_p[], double cross_c[], double *X, double *Y, double *Z){
    
    double nVe,nGl;
	nGl=sqrt(pow(G.vec_x,2.)+pow(G.vec_y,2.)+pow(G.vec_z,2.));
	nVe=sqrt( pow(cross_p[0]-(cross_c[0]-mm.d[0]*G.vec_x/nGl),2.)
		     +pow(cross_p[1]-(cross_c[1]-mm.d[0]*G.vec_y/nGl),2.)
			 +pow(cross_p[2]-(cross_c[2]-mm.d[0]*G.vec_z/nGl),2.));

	*X = cross_c[0]-mm.d[0]*G.vec_x/nGl + Z_t*G.vec_x/nGl;
	*Y = cross_c[1]-mm.d[0]*G.vec_y/nGl + Z_t*G.vec_y/nGl;
	*Z = cross_c[2]-mm.d[0]*G.vec_z/nGl + Z_t*G.vec_z/nGl;

    if (nVe > 0) {  
        // We need this for when the cam-point line is exactly perp. to glass.
        // The degenerate case will have nVe == 0 and produce NaNs on the
        // following calculations.
        *X += X_t*(cross_p[0] - (cross_c[0] - mm.d[0]*G.vec_x/nGl))/nVe;
        *Y += X_t*(cross_p[1] - (cross_c[1] - mm.d[0]*G.vec_y/nGl))/nVe;
        *Z += X_t*(cross_p[2] - (cross_c[2] - mm.d[0]*G.vec_z/nGl))/nVe;
    }
}

/* calculates and returns the radial shift */
double multimed_r_nlay (Exterior ex, Exterior ex_o, mm_np mm, double X, double Y,\
double Z, int cam){

  int   i, it=0;
  double beta1, beta2[32], beta3, r, rbeta, rdiff, rq, mmf;
  double ocf=1.0; // over compensation factor for faster convergence 

  double absError=100;
  int counter=0;
  double dir_water_x=-X;
  double dir_water_z=ex.z0-Z;
  double dist=pow(dir_water_x*dir_water_x+dir_water_z*dir_water_z,0.5);
  double xInInterFace,comp_parallel,comp_perpendicular,dir_air_x,dir_air_z,error_x,error_z;
  mmlut  *mmLUT;
  
  dir_water_x=dir_water_x/dist;
  dir_water_z=dir_water_z/dist;
  
  
  
  // 1-medium case 
  if (mm.n1==1 && mm.nlay == 1 && mm.n2[0]==1 && mm.n3==1) return (1.0);
  
  
  // interpolation in mmLUT, if selected (requires some global variables) 
  if (mm.lut) {
    mmf = get_mmf_from_mmLUT (cam, X,Y,Z, (mmlut *)mmLUT);
    if (mmf > 0) return (mmf);
  }
 
  // iterative procedure 
  r = sqrt ((X-ex.x0)*(X-ex.x0)+(Y-ex.y0)*(Y-ex.y0));
  rq = r;
  
  do
    {
      beta1 = atan (rq/(ex.z0-Z));
      for (i=0; i<mm.nlay; i++) beta2[i] = asin (sin(beta1) * mm.n1/mm.n2[i]);
      beta3 = asin (sin(beta1) * mm.n1/mm.n3);
      
      rbeta = (ex.z0-mm.d[0]) * tan(beta1) - Z * tan(beta3);
      for (i=0; i<mm.nlay; i++) rbeta += (mm.d[i] * tan(beta2[i]));
      rdiff = r - rbeta;
      rdiff *= ocf;
      rq += rdiff;
      it++;
    }
  while (((rdiff > 0.001) || (rdiff < -0.001))  &&  it < 40);
  
  if (it >= 40)
    {
      puts ("Multimed_r_nlay_v2 stopped after 40. Iteration");  return (1.0);
    }
  
  if (r != 0)   return (rq/r);  else return (1.0);
}

/* init_mmLUT prepares the Look-Up Table
Arguments:

*/ 
void init_mmLUT (volume_par *vpar
               , control_par *cpar
               , Calibration *cal
               , mmlut *mmLUT){

  register int  i,j, nr, nz, i_cam;
  double        X,Y,Z, R, Zmin, Rmax=0, Zmax;
  double		pos[3], a[3]; 
  double        x,y, *Ri,*Zi;
  double        rw = 2.0; 
  Exterior      Ex_t[4];
  double        X_t,Y_t,Z_t, Zmin_t,Zmax_t;
  double        cross_p[3],cross_c[3]; 
  FILE          *fpp;
  double        xc[2], yc[2];  /* image corners */
  
  
  xc[0] = 0.0;
  xc[1] = (double) cpar->imx;
  yc[0] = 0.0;
  yc[1] = (double) cpar->imy;

 
  /* find extrema of imaged object volume */
  /* ==================================== */
  
  /* find extrema in depth */
  
  /* move the loop through n_cams inside the function, i.e.
  TODO: in jw_ptv.c change the call from:
  for (i_img = 0; i_img < cpar->num_cams; i_img++) init_mmLUT(i_img);
  to:
  
  init_mmLUT(vpar,cpar,cal,ap,mmLUT);
  */
  
  for (i_cam = 0; i_cam < cpar->num_cams; i_cam++){
  
	  Zmin -= fmod (vpar->Zmin_lay[0], rw);
	  Zmax += (rw - fmod (vpar->Zmax_lay[0], rw));
	  Zmin_t = Zmin;
	  Zmax_t = Zmax;
  
	  /* intersect with image vertices rays */

	  /* 0,0 pix corner */
	  

	  
	  for (i = 0; i < 2; i ++) for (j = 0; j < 2; j++) {
	  	  
		  pixel_to_metric (&x, &y, xc[i], yc[j], cpar);
		  /* x = x - I[i_cam].xh;
			 y = y - I[i_cam].yh;
		  */
		  x = x - cal[i_cam].int_par.xh;
		  y = y - cal[i_cam].int_par.yh;
  
		  correct_brown_affin (x, y, cal[i_cam].added_par, &x,&y);
	  
		  /* ray_tracing(x,y, Ex[i_cam], I[i_cam], G[i_cam], mmp, &X1, &Y1, &Z1, &a, &b, &c); */
		  ray_tracing(x,y, &cal[i_cam], *(cpar->mm), pos, a);
  
		  /* Z = Zmin;   X = X1 + (Z-Z1) * a/c;   Y = Y1 + (Z-Z1) * b/c; */
		  Z = Zmin;   
		  X = pos[0] + (Z - pos[2]) * a[0]/a[2];   
		  Y = pos[1] + (Z - pos[2]) * a[1]/a[2];
	  
		  /* trans */
		  /* trans_Cam_Point(Ex[i_cam],mmp,G[i_cam],X,Y,Z,&Ex_t[i_cam],&X_t,&Y_t,&Z_t,&cross_p,&cross_c); */
		  trans_Cam_Point(cal[i_cam].ext_par, *(cpar->mm), cal[i_cam].glass_par, X, Y, Z, \
		  &Ex_t[i_cam], &X_t, &Y_t, &Z_t, (double *)cross_p, (double *)cross_c);
	  
		  if( Z_t < Zmin_t ) Zmin_t = Z_t;
		  if( Z_t > Zmax_t ) Zmax_t = Z_t;
	  
	  
		  R = sqrt (( X_t - Ex_t[i_cam].x0 ) * ( X_t - Ex_t[i_cam].x0 )
				  + ( Y_t - Ex_t[i_cam].y0 ) * ( Y_t - Ex_t[i_cam].y0 )); 
  
		  if (R > Rmax) Rmax = R;
	  
		  /* Z = Zmax;   X = X1 + (Z-Z1) * a/c;   Y = Y1 + (Z-Z1) * b/c; */
		  Z = Zmax;   
		  X = pos[0] + (Z - pos[2]) * a[0]/a[2];   
		  Y = pos[1] + (Z - pos[2]) * a[1]/a[2];
	  
		  /* trans */
		  /* trans_Cam_Point(Ex[i_cam],mmp,G[i_cam],X,Y,Z,&Ex_t[i_cam],&X_t,&Y_t,&Z_t,&cross_p,&cross_c); */
		  trans_Cam_Point(cal[i_cam].ext_par, *(cpar->mm), cal[i_cam].glass_par, X, Y, Z, \
		  &Ex_t[i_cam], &X_t, &Y_t, &Z_t, (double *)cross_p, (double *)cross_c);
	  
		  if( Z_t < Zmin_t ) Zmin_t = Z_t;
		  if( Z_t > Zmax_t ) Zmax_t = Z_t;
	  
	  
		  R = sqrt (( X_t - Ex_t[i_cam].x0 ) * ( X_t - Ex_t[i_cam].x0 )
				  + ( Y_t - Ex_t[i_cam].y0 ) * ( Y_t - Ex_t[i_cam].y0 )); 
  
	  
		  if (R > Rmax) Rmax = R;
	  }
  
	  /* round values (-> enlarge) */
	  Rmax += (rw - fmod (Rmax, rw));
	
	  /* get # of rasterlines in r,z */
	  nr = Rmax/rw + 1;
	  nz = (Zmax_t - Zmin_t)/rw + 1;
 
	  /* create two dimensional mmLUT structure */
	  
	  trans_Cam_Point(cal[i_cam].ext_par, *(cpar->mm), cal[i_cam].glass_par, X, Y, Z, \
		  &Ex_t[i_cam], &X_t, &Y_t, &Z_t, (double *)cross_p, (double *)cross_c);

	  mmLUT[i_cam].origin.x = Ex_t[i_cam].x0;
	  mmLUT[i_cam].origin.y = Ex_t[i_cam].y0;
	  mmLUT[i_cam].origin.z = Zmin_t;
	  mmLUT[i_cam].nr = nr;
	  mmLUT[i_cam].nz = nz;
	  mmLUT[i_cam].rw = rw;
	  mmLUT[i_cam].data = (double *) malloc (nr*nz * sizeof (double));
   
	  /* fill mmLUT structure */
	  /* ==================== */
  
	  Ri = (double *) malloc (nr * sizeof (double));
	  for (i=0; i<nr; i++)  Ri[i] = i*rw;
	  Zi = (double *) malloc (nz * sizeof (double));
	  for (i=0; i<nz; i++)  Zi[i] = Zmin_t + i*rw;
  
	  for (i=0; i<nr; i++)  for (j=0; j<nz; j++)
		{
		//old mmLUT[i_cam].data[i*nz + j]= multimed_r_nlay (Ex[i_cam], mmp, 
		//                                                  Ri[i]+Ex[i_cam].x0, Ex[i_cam].y0, Zi[j]);
		//trans
		trans_Cam_Point(cal[i_cam].ext_par, *(cpar->mm), cal[i_cam].glass_par, X, Y, Z, \
		  &Ex_t[i_cam], &X_t, &Y_t, &Z_t, (double *)cross_p, (double *)cross_c);
		  
		  mmLUT[i_cam].data[i*nz + j]
		= multimed_r_nlay (Ex_t[i_cam], cal[i_cam].ext_par, *(cpar->mm), 
							  Ri[i] + Ex_t[i_cam].x0, Ex_t[i_cam].y0, Zi[j], i_cam);
		} /* for nr,nz */
    } /* for n_cams */
}



double get_mmf_from_mmLUT (int i_cam, double X, double Y, double Z, mmlut *mmLUT){

  int       i, ir,iz, nr,nz, v4[4];
  double    R, sr,sz, rw, mmf=1;
  
  rw =  mmLUT[i_cam].rw;
  
  Z -= mmLUT[i_cam].origin.z; sz = Z/rw; iz = (int) sz; sz -= iz;
  X -= mmLUT[i_cam].origin.x;
  Y -= mmLUT[i_cam].origin.y;
  R = sqrt (X*X + Y*Y); sr = R/rw; ir = (int) sr; sr -= ir;
    
  nz =  mmLUT[i_cam].nz;
  nr =  mmLUT[i_cam].nr;
    
  /* check whether point is inside camera's object volume */
  if (ir > nr)              return (0);
  if (iz < 0  ||  iz > nz)  return (0);
  
  /* bilinear interpolation in r/z box */
  /* ================================= */
  
  /* get vertices of box */
  v4[0] = ir*nz + iz;
  v4[1] = ir*nz + (iz+1);
  v4[2] = (ir+1)*nz + iz;
  v4[3] = (ir+1)*nz + (iz+1);
  
  /* 2. check wther point is inside camera's object volume */
  /* important for epipolar line computation */
  for (i=0; i<4; i++)
    if (v4[i] < 0  ||  v4[i] > nr*nz)   return (0);
  
  /* interpolate */
  mmf = mmLUT[i_cam].data[v4[0]] * (1-sr)*(1-sz)
    + mmLUT[i_cam].data[v4[1]] * (1-sr)*sz
    + mmLUT[i_cam].data[v4[2]] * sr*(1-sz)
    + mmLUT[i_cam].data[v4[3]] * sr*sz;
  
  return (mmf);
}



void volumedimension (double *xmax
					, double *xmin
					, double *ymax
					, double *ymin
					, double *zmax
					, double *zmin
					, volume_par *vpar
               		, control_par *cpar
               		, Calibration *cal){

  int   i_cam, i, j;
  double X,Y,Z, R, Zmin, Rmax=0,Zmax;
  double pos[3], a[3];
  double x,y;  
  double        xc[2], yc[2];  /* image corners */
  
  xc[0] = 0.0;
  xc[1] = (double) cpar->imx;
  yc[0] = 0.0;
  yc[1] = (double) cpar->imy;
  
/*
  *zmin = Zmin;
  *zmax = Zmax;
*/ 
   *zmin = vpar->Zmin_lay[0];
   *zmax = vpar->Zmax_lay[0]; 

    
  /* find extrema of imaged object volume */
  /* ==================================== */
  


  for (i_cam = 0; i_cam < cpar->num_cams; i_cam++){
  	for (i = 0; i < 2; i ++) for (j = 0; j < 2; j++) {

  
      /* intersect with image vertices rays */
/*
          pixel_to_metric (0.0, 0.0, imx,imy, pix_x,pix_y, &x,&y, chfield);
		  x = x - I[i_cam].xh;
		  y = y - I[i_cam].yh;
		  correct_brown_affin (x, y, ap[i_cam], &x,&y);
		  ray_tracing(x,y, Ex[i_cam], I[i_cam], G[i_cam], mmp, &X1, &Y1, &Z1, &a, &b, &c);
		  Z = Zmin;   X = X1 + (Z-Z1) * a/c;   Y = Y1 + (Z-Z1) * b/c;
		  R = sqrt (  (X-Ex[i_cam].x0)*(X-Ex[i_cam].x0)
			  + (Y-Ex[i_cam].y0)*(Y-Ex[i_cam].y0)); 

*/ 
          pixel_to_metric (&x, &y, xc[i], yc[j], cpar);
          x = x - cal[i_cam].int_par.xh;
		  y = y - cal[i_cam].int_par.yh;
  
		  correct_brown_affin (x, y, cal[i_cam].added_par, &x,&y);
	  	  ray_tracing(x,y, &cal[i_cam], *(cpar->mm), pos, a);
  		  Z = Zmin;   
		  X = pos[0] + (Z - pos[2]) * a[0]/a[2];   
		  Y = pos[1] + (Z - pos[2]) * a[1]/a[2];
		  R = sqrt (( X - cal[i_cam].ext_par.x0 ) * ( X - cal[i_cam].ext_par.x0 )
				  + ( Y - cal[i_cam].ext_par.y0 ) * ( Y - cal[i_cam].ext_par.y0 ));
		  
 
		  if ( X > *xmax) *xmax=X;
		  if ( X < *xmin) *xmin=X;
		  if ( Y > *ymax) *ymax=Y;
		  if ( Y < *ymin) *ymin=Y;

          if (R > Rmax) Rmax = R;
      
/* 
      
      Z = Zmax;   X = X1 + (Z-Z1) * a/c;   Y = Y1 + (Z-Z1) * b/c;
      R = sqrt (  (X-Ex[i_cam].x0)*(X-Ex[i_cam].x0)
          + (Y-Ex[i_cam].y0)*(Y-Ex[i_cam].y0));
*/
		
		Z = Zmax;
	    X = pos[0] + (Z - pos[2]) * a[0]/a[2];   
		Y = pos[1] + (Z - pos[2]) * a[1]/a[2];
		R = sqrt (( X - cal[i_cam].ext_par.x0 ) * ( X - cal[i_cam].ext_par.x0 )
				  + ( Y - cal[i_cam].ext_par.y0 ) * ( Y - cal[i_cam].ext_par.y0 )); 

      if ( X > *xmax) *xmax=X;
      if ( X < *xmin) *xmin=X;
      if ( Y > *ymax) *ymax=Y;
      if ( Y < *ymin) *ymin=Y;
      
      if (R > Rmax) Rmax = R;
      
      }
   }
}
