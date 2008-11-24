/* FILE: CalculateE.c
 * AUTH: Alfons Hoekstra
 * DESCR: The module that will calculate the E field, it is very messy with
 *        global variables etc. That must be straightened out later, (no way)
 *        first we concentrate on the harness of the implementation...
 *        
 *        January 2004 : include module to compute full Mueller Matrix over
 *        full space angle, not very efficient, must be improved (A. Hoekstra)
 *        
 *        Currently is developed by Maxim Yurkin
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "cmplx.h"
#include "const.h"
#include "comm.h"
#include "debug.h"
#include "crosssec.h"

extern int nDip; 		/* defined in calculator */		
extern int nTheta;		/* defined in calculator */
extern double *DipoleCoord;	/* defined in calculator */
extern doublecomplex *Avecbuffer;	/* defined in calculator */
extern double WaveNum;		/* defined in calculator */

extern int yzplane;		/* defined in main */
extern int xzplane;		/* defined in main */
extern int store_all_dir;
extern int all_dir;		/* defined in main */
extern int calc_Cext;		/* defined in main */
extern int calc_Cabs; 		/* defined in main */
extern int calc_Csca;        	/* defined in main */
extern int calc_Csca_diff;      /* defined in main */
extern int calc_vec;            /* defined in main */
extern int calc_asym; 		/* defined in main */
extern int calc_mat_force;      /* defined in main */
extern int store_force;

extern doublecomplex *x,*p;
extern doublecomplex *Eper, *Epar;
extern double *Eparper_buffer;
extern doublecomplex *Einc;

extern char directory[200];
extern FILE *logfile;
extern int store_int_field;
extern double gridspace;
extern int Nmat;
extern char *material;

extern doublecomplex ref_index[MAXNMAT];
extern int IterMethod;

extern int symR;
extern double dpl;
extern integr_parms alpha_int;
extern int orient_avg;
extern double alph;
extern doublecomplex *ampl_alpha;
extern double *muel_alpha;
extern clock_t Timing_EField, Timing_FileIO, Timing_IntField,
    Timing_EFieldPlane,Timing_calc_EField,Timing_comm_EField,Timing_ScatQuan,
    Timing_IntFieldOne;

clock_t tstart_CE;

extern void GenerateB(char which,doublecomplex *x);
extern int iterative_solver(int method);

/*============================================================*/

void compute_mueller_matrix(double matrix[4][4], doublecomplex s1, doublecomplex s2, doublecomplex s3, doublecomplex s4)
/* computer mueller matrix from scattering matrix elements s1, s2, s3, s4, accoording 
   to formula 3.16 from Bohren and Huffman */
{
	matrix[0][0] = 0.5*(cMultConRe(s1,s1)+cMultConRe(s2,s2)+
	                    cMultConRe(s3,s3)+cMultConRe(s4,s4));
	matrix[0][1] = 0.5*(cMultConRe(s2,s2)-cMultConRe(s1,s1)+
	                    cMultConRe(s4,s4)-cMultConRe(s3,s3));
	matrix[0][2] = cMultConRe(s2,s3)+cMultConRe(s1,s4);
	matrix[0][3] = cMultConIm(s2,s3)-cMultConIm(s1,s4);
	
	matrix[1][0] = 0.5*(cMultConRe(s2,s2)-cMultConRe(s1,s1)+
	                    cMultConRe(s3,s3)-cMultConRe(s4,s4));
	matrix[1][1] = 0.5*(cMultConRe(s2,s2)+cMultConRe(s1,s1)+
	                    -cMultConRe(s3,s3)-cMultConRe(s4,s4));
	matrix[1][2] = cMultConRe(s2,s3)-cMultConRe(s1,s4);
	matrix[1][3] = cMultConIm(s2,s3)+cMultConIm(s1,s4);

	matrix[2][0] = cMultConRe(s2,s4)+cMultConRe(s1,s3);
	matrix[2][1] = cMultConRe(s2,s4)-cMultConRe(s1,s3);
	matrix[2][2] = cMultConRe(s1,s2)+cMultConRe(s3,s4);
	matrix[2][3] = cMultConIm(s2,s1)+cMultConIm(s4,s3);

	matrix[3][0] = cMultConIm(s4,s2)+cMultConIm(s1,s3);
	matrix[3][1] = cMultConIm(s4,s2)-cMultConIm(s1,s3);
	matrix[3][2] = cMultConIm(s1,s2)-cMultConIm(s3,s4);
	matrix[3][3] = cMultConRe(s1,s2)-cMultConRe(s3,s4);
}

/*============================================================*/

void compute_mueller_matrix_norm(double matrix[4][4], doublecomplex s1, doublecomplex s2, doublecomplex s3, doublecomplex s4)
/* computer mueller matrix from scattering matrix elements s1, s2, s3, s4, accoording to 
   formula 3.16 from Bohren and Huffman; normalize all elements to S11 (except itself)*/
{
	matrix[0][0] = 0.5*(cMultConRe(s1,s1)+cMultConRe(s2,s2)+
	                    cMultConRe(s3,s3)+cMultConRe(s4,s4));
	matrix[0][1] = 0.5*(cMultConRe(s2,s2)-cMultConRe(s1,s1)+
	                    cMultConRe(s4,s4)-cMultConRe(s3,s3))/matrix[0][0];
	matrix[0][2] = (cMultConRe(s2,s3)+cMultConRe(s1,s4))/matrix[0][0];
	matrix[0][3] = (cMultConIm(s2,s3)-cMultConIm(s1,s4))/matrix[0][0];
	
	matrix[1][0] = 0.5*(cMultConRe(s2,s2)-cMultConRe(s1,s1)+
	                    cMultConRe(s3,s3)-cMultConRe(s4,s4))/matrix[0][0];
	matrix[1][1] = 0.5*(cMultConRe(s2,s2)+cMultConRe(s1,s1)+
	                    -cMultConRe(s3,s3)-cMultConRe(s4,s4))/matrix[0][0];
	matrix[1][2] = (cMultConRe(s2,s3)-cMultConRe(s1,s4))/matrix[0][0];
	matrix[1][3] = (cMultConIm(s2,s3)+cMultConIm(s1,s4))/matrix[0][0];

	matrix[2][0] = (cMultConRe(s2,s4)+cMultConRe(s1,s3))/matrix[0][0];
	matrix[2][1] = (cMultConRe(s2,s4)-cMultConRe(s1,s3))/matrix[0][0];
	matrix[2][2] = (cMultConRe(s1,s2)+cMultConRe(s3,s4))/matrix[0][0];
	matrix[2][3] = (cMultConIm(s2,s1)+cMultConIm(s4,s3))/matrix[0][0];

	matrix[3][0] = (cMultConIm(s4,s2)+cMultConIm(s1,s3))/matrix[0][0];
	matrix[3][1] = (cMultConIm(s4,s2)-cMultConIm(s1,s3))/matrix[0][0];
	matrix[3][2] = (cMultConIm(s1,s2)-cMultConIm(s3,s4))/matrix[0][0];
	matrix[3][3] = (cMultConRe(s1,s2)-cMultConRe(s3,s4))/matrix[0][0];
}

/*==============================================================*/

void mueller_matrix(void)
{
  FILE *mueller,*parpar,*parper,*perper,*perpar, *E_Y, *E_X;
  double matrix[4][4];
  double theta, phi;
  double exr, exi, eyr, eyi, ezr, ezi;
  double sphi, cphi, stheta, ctheta;
  double exper_r, exper_i, expar_r, expar_i;
  double eyper_r, eyper_i, eypar_r, eypar_i;
  doublecomplex s1,s2,s3,s4,s10,s20,s30,s40;
  char stringbuffer[500];
  int index,index1,nth2,nth3,i,k_or;
  double co,si;
  clock_t tstart;

  if (ringid!=ROOT) return;

  if (orient_avg) {                     /* Amplitude matrix stored in ampl_alplha is */
    index1=index=0;                     /* transformed into Mueller matrix stored in muel_alpha */
    nth2=2*nTheta;
    nth3=3*nTheta;
    for (k_or=0;k_or<alpha_int.N;k_or++) {
      alph=deg2rad(alpha_int.val[k_or]);     /* read current alpha */
      co=cos(alph);
      si=sin(alph);
      for (i=0;i<nTheta;i++) {
        memcpy(s10,&ampl_alpha[index],sizeof(doublecomplex));               /* read amplitude matrix from memory */
        memcpy(s20,&ampl_alpha[index+nTheta],sizeof(doublecomplex));
        memcpy(s30,&ampl_alpha[index+nth2],sizeof(doublecomplex));
        memcpy(s40,&ampl_alpha[index+nth3],sizeof(doublecomplex));
                                             /* transform it, multiplying by rotation matrix (-alpha) */
        cLinComb(s20,s30,co,si,s2);         /* s2 =  co*s20 + si*s30  */
        cLinComb(s20,s30,-si,co,s3);        /* s3 = -si*s20 + co*s30  */
        cLinComb(s40,s10,co,si,s4);         /* s4 =  co*s40 + si*s10  */
        cLinComb(s40,s10,-si,co,s1);        /* s1 = -si*s40 + co*s10  */

        compute_mueller_matrix((double (*)[4])(muel_alpha+index1),s1,s2,s3,s4);
        index++;
        index1+=16;
      }
      index+=nth3;
    }
  }
  else {                                /* here amplitude matrix is read from file */
    tstart=clock();                    /* and Mueller matrix is saved to file     */
    if (yzplane) {
      strcpy(stringbuffer,directory);
      strcat(stringbuffer,"/per-par");
      if(xzplane==true) strcat(stringbuffer,"-yz");
      perpar = fopen (stringbuffer, "r");
      strcpy(stringbuffer,directory);
      strcat(stringbuffer,"/per-per");
      if(xzplane==true) strcat(stringbuffer,"-yz");
      perper = fopen (stringbuffer, "r");
      strcpy(stringbuffer,directory);
      strcat(stringbuffer,"/par-par");
      if(xzplane==true) strcat(stringbuffer,"-yz");
      parpar = fopen (stringbuffer, "r");
      strcpy(stringbuffer,directory);
      strcat(stringbuffer,"/par-per");
      if(xzplane==true) strcat(stringbuffer,"-yz");
      parper = fopen (stringbuffer, "r");

      if (parpar==NULL || parper==NULL || perper==NULL || perpar==NULL)
        LogError(EC_ERROR,ONE,POSIT,"File read error (per-par,...)");

      strcpy(stringbuffer,directory); strcat(stringbuffer,"/mueller");
      if(xzplane==true) strcat(stringbuffer,"-yz");
      mueller = fopen (stringbuffer, "w");
      if (mueller==NULL) LogError(EC_ERROR,ONE,POSIT,"File 'mueller' write error");

      fprintf(mueller,"theta s11 s12 s13 s14 s21 s22 s23 s24 s31 s32 s33 s34 s41 s42 s43 s44\n");

      while(!feof(parpar)) { /* not debugged */
	fscanf(parpar,"%lf %lf %lf\n",&theta,&s2[re],&s2[im]);
	fscanf(parper,"%lf %lf %lf\n",&theta,&s4[re],&s4[im]);
	fscanf(perper,"%lf %lf %lf\n",&theta,&s1[re],&s1[im]);
	fscanf(perpar,"%lf %lf %lf\n",&theta,&s3[re],&s3[im]);

	compute_mueller_matrix(matrix,s1,s2,s3,s4);

	fprintf(mueller,
		"%.2f %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E\n",
		theta,
		matrix[0][0],
		matrix[0][1],
		matrix[0][2],
		matrix[0][3],
		matrix[1][0],
		matrix[1][1],
		matrix[1][2],
		matrix[1][3],
		matrix[2][0],
		matrix[2][1],
		matrix[2][2],
		matrix[2][3],
		matrix[3][0],
		matrix[3][1],
		matrix[3][2],
		matrix[3][3]);
      }
      fclose(parpar);
      fclose(parper);
      fclose(perper);
      fclose(perpar);
      fclose(mueller);
#ifndef DEBUG
      /* delete temporary files */
      sprintf(stringbuffer,"%s/par-par",directory);
      remove(stringbuffer);
      sprintf(stringbuffer,"%s/par-per",directory);
      remove(stringbuffer);
      sprintf(stringbuffer,"%s/per-par",directory);
      remove(stringbuffer);
      sprintf(stringbuffer,"%s/per-per",directory);
      remove(stringbuffer);
#endif
    }

    if(xzplane==true) {
      /* also calculate mueller in xz plane */
      strcpy(stringbuffer,directory); 
      strcat(stringbuffer,"/per-par-xz");
      perpar = fopen (stringbuffer, "r");
      strcpy(stringbuffer,directory); 
      strcat(stringbuffer,"/per-per-xz");
      perper = fopen (stringbuffer, "r");
      strcpy(stringbuffer,directory); 
      strcat(stringbuffer,"/par-par-xz");
      parpar = fopen (stringbuffer, "r");
      strcpy(stringbuffer,directory); 
      strcat(stringbuffer,"/par-per-xz");
      parper = fopen (stringbuffer, "r");
      
      if (parpar==NULL || parper==NULL || perper==NULL || perpar==NULL) 
        LogError(EC_ERROR,ONE,POSIT,"File read error (per-par-xz,...)");

      strcpy(stringbuffer,directory); strcat(stringbuffer,"/mueller-xz");
      mueller = fopen (stringbuffer, "w");
      if (mueller==NULL) LogError(EC_ERROR,ONE,POSIT,"File 'mueller-xz' write error");

      fprintf(mueller,"theta s11 s12 s13 s14 s21 s22 s23 s24 s31 s32 s33 s34 s41 s42 s43 s44\n");

      while(!feof(parpar)) { /* not debugged */
	fscanf(parpar,"%lf %lf %lf\n",&theta,&s2[re],&s2[im]);
	fscanf(parper,"%lf %lf %lf\n",&theta,&s4[re],&s4[im]);
	fscanf(perper,"%lf %lf %lf\n",&theta,&s1[re],&s1[im]);
	fscanf(perpar,"%lf %lf %lf\n",&theta,&s3[re],&s3[im]);
	
	compute_mueller_matrix(matrix,s1,s2,s3,s4);

	fprintf(mueller,
		"%.2f %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E\n",
		theta,
		matrix[0][0],
		matrix[0][1],
		matrix[0][2],
		matrix[0][3],
		matrix[1][0],
		matrix[1][1],
		matrix[1][2],
		matrix[1][3],
		matrix[2][0],
		matrix[2][1],
		matrix[2][2],
		matrix[2][3],
		matrix[3][0],
		matrix[3][1],
		matrix[3][2],
		matrix[3][3]);

      }
      fclose(parpar); 
      fclose(parper);
      fclose(perper);
      fclose(perpar);
      fclose(mueller);
#ifndef DEBUG
      /* delete temporary files */
      sprintf(stringbuffer,"%s/par-par-xz",directory);
      remove(stringbuffer);
      sprintf(stringbuffer,"%s/par-per-xz",directory);
      remove(stringbuffer);
      sprintf(stringbuffer,"%s/per-par-xz",directory);
      remove(stringbuffer);
      sprintf(stringbuffer,"%s/per-per-xz",directory);
      remove(stringbuffer);
#endif
    }

    if(all_dir && store_all_dir){
      /* compute Mueller Matrix in full space angle. E-fields are stored in file EgridX and EgridY */
      /* for incoming X and Y polarized light. From this compute the Mueller matrix. */
      /* this is done by first computing the scattered fields in the par-per frame, using */
      /* Espar = cos(theta)cos(phi)Ex + cos(theta)sin(phi)Ey - sin(theta)Ez */
      /* Esper = sin(phi)Ex - cos(phi)Ey */
      /* Next, this is converted to the scattering matrix elements (see e.g Bohren and Huffman) : */
      /* s2 = cos(phi)E'X'par + sin(phi)E'Y'par */
      /* s3 = sin(phi)E'X'par - cos(phi)E'Y'par */
      /* s4 = cos(phi)E'X'per + sin(phi)E'Y'per */
      /* s1 = sin(phi)E'X'per - cos(phi)E'Y'per */
      /* from this the mueller matrix elements are computed */
      strcpy(stringbuffer,directory); 
      strcat(stringbuffer,"/EgridX");
      E_X = fopen(stringbuffer, "r");
      strcpy(stringbuffer,directory);
      strcat(stringbuffer,"/EgridY");
      E_Y = fopen(stringbuffer, "r");

      if (E_X==NULL || E_Y==NULL) LogError(EC_ERROR,ONE,POSIT,"File read error (EgridX or EgridY)");

      strcpy(stringbuffer,directory);
      strcat(stringbuffer,"/mueller-AllDir");
      mueller=fopen(stringbuffer, "w");
      if (mueller==NULL) LogError(EC_ERROR,ONE,POSIT,"File 'mueller-AllDir' write error");

      fprintf(mueller,"theta phi s11 s12 s13 s14 s21 s22 s23 s24 s31 s32 s33 s34 s41 s42 s43 s44\n");
      while(!feof(E_Y)) { /* not debugged */
	fscanf(E_X,"%lf %lf %lf %lf %lf %lf %lf %lf\n",&theta,&phi,&exr,&exi,&eyr,&eyi,&ezr,&ezi);
	sphi = sin(phi*PI/180.0); cphi = cos(phi*PI/180.0);
	stheta = sin(theta*PI/180.0); ctheta = cos(theta*PI/180.0);
	expar_r = ctheta*cphi*exr + ctheta*sphi*eyr - stheta*ezr;
	expar_i = ctheta*cphi*exi + ctheta*sphi*eyi - stheta*ezi;
	exper_r = sphi*exr - cphi*eyr;
	exper_i = sphi*exi - cphi*eyi;
	fscanf(E_Y,"%lf %lf %lf %lf %lf %lf %lf %lf\n",&theta,&phi,&exr,&exi,&eyr,&eyi,&ezr,&ezi);
	eypar_r = ctheta*cphi*exr + ctheta*sphi*eyr - stheta*ezr;
	eypar_i = ctheta*cphi*exi + ctheta*sphi*eyi - stheta*ezi;
	eyper_r = sphi*exr - cphi*eyr;
	eyper_i = sphi*exi - cphi*eyi;

	s2[re] = cphi*expar_r + sphi*eypar_r;
	s2[im] = cphi*expar_i + sphi*eypar_i;
	s3[re] = sphi*expar_r - cphi*eypar_r;
	s3[im] = sphi*expar_i - cphi*eypar_i;
	s4[re] = cphi*exper_r + sphi*eyper_r;
	s4[im] = cphi*exper_i + sphi*eyper_i;
	s1[re] = sphi*exper_r - cphi*eyper_r;
	s1[im] = sphi*exper_i - cphi*eyper_i;
	
	compute_mueller_matrix(matrix,s1,s2,s3,s4);

	fprintf(mueller,
		"%.2f %.2f %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E %.7E\n",
		theta,
		phi,
		matrix[0][0],
		matrix[0][1],
		matrix[0][2],
		matrix[0][3],
		matrix[1][0],
		matrix[1][1],
		matrix[1][2],
		matrix[1][3],
		matrix[2][0],
		matrix[2][1],
		matrix[2][2],
		matrix[2][3],
		matrix[3][0],
		matrix[3][1],
		matrix[3][2],
		matrix[3][3]);
      }
      fclose(E_X); fclose(E_Y);
      fclose(mueller);
#ifndef DEBUG
      /* delete temporary files */
      sprintf(stringbuffer,"%s/EgridX",directory);
      remove(stringbuffer);
      sprintf(stringbuffer,"%s/EgridY",directory);
      remove(stringbuffer);
#endif
    }
    Timing_FileIO += clock() - tstart;
  }
}

/*============================================================*/

void CalculateE(char which,int type) /* x or y polarized incident light */
{
  int i, j;
  doublecomplex ebuff[3];	/* small vector to hold E fields */
  double robserver[3];	/* small vector for observer in E calculation */
  double epar[3];       /* unit vector in direction of Epar */
  double theta, dtheta;	/* scattering angle and step in it */
  FILE *filepar, *fileper; /* files for calculated fields */
  FILE *Intfldpntr;	   /* file to store internal fields */
  char stringbuffer[500];
  double co,si;          /* temporary, cos and sin of some angle */
  extern double prop[3],incPolX[3],incPolY[3]; /* incident propagation and polarization */

  double incPol_tmp1[3],incPol_tmp2[3]; /* just allocateed memory for incPolper, incPolpar */
  double *incPol,*incPolper,*incPolpar;

  clock_t tstart, tstart2, tiostart, tiostop;

  extern unsigned long TotalEFieldPlane;
  int k_or;

  int nmax;
  int orient,Norient;
  extern int symY,symX;
  char whichfile;
  int index;

  double *Fsca,*Finc,*Frp; /* Scattering force, extinction force and
  		             radiation pressure per dipole */
  double Cext,Cabs,Csca,   /* Cross sections */
         g[3],             /* asymmetry paramter */
         dummy[3],
         Fsca_tot[3],      /* total scattering force */
         Finc_tot[3],      /* total extinction force */
         Frp_tot[3],       /* total radiation pressure */
         Cnorm,            /* normalizing factor from force to cross section */
         Qnorm,            /* normalizing factor from force to efficiency */
         a_eq,             /* equivalent sphere radius */
         G;                /* cross section surface */
  FILE *VisFrp,*CCfile;
  /* used in store_int_field */
  double V;
  doublecomplex hi,hi_inv[MAXNMAT],fld[3];
  char mat;

  tstart_CE=clock();

  /* calculate the incident field Einc; vector b=Einc*cc_sqrt */
  D("Generating B");
  GenerateB (which, Einc);

  /* calculate solution vector x */
  D("Iterative solver started");
  iterative_solver(IterMethod);

  Timing_IntFieldOne = clock() - tstart_CE;
  Timing_IntField += Timing_IntFieldOne;

  incPolper=incPol_tmp1;  /* initialization of per and par polarizations */
  incPolpar=incPol_tmp2;

  /* CALCULATE THE ELECTIC FIELD FROM X */
  /* 1: for all angles, summate the condtributions of all dipoles in
   *    this processor;
   * 2: accumulate all local fields and summate
   */

  if (yzplane) {  /*generally plane of incPolY and prop*/
    for (k_or=0;k_or<alpha_int.N;k_or++) {
      /* cycle over alpha - for orientation averaging */
      if (orient_avg) {
        alph=deg2rad(alpha_int.val[k_or]);          /* rotate polarization basis vectors by -alpha */
        co=cos(alph);
        si=sin(alph);
        LinComb(incPolX,incPolY,co,-si,incPolper);  /* incPolper = co*incPolX - si*incPolY; */
        LinComb(incPolX,incPolY,si,co,incPolpar);   /* incPolpar = si*incPolX + co*incPolY; */
      }
      else {
        memcpy(incPolper,incPolX,3*sizeof(double));   /* per <=> X */
        memcpy(incPolpar,incPolY,3*sizeof(double));   /* par <=> Y */
      }
      if (type==NORMAL) Norient=1; else Norient=2; /* # orientations */

      dtheta = PI / ((double)(nTheta-1));

      for(orient=0;orient<Norient;orient++) {
        /* in case of Rotation symmetry */
        tstart = clock ();

        if (orient==0) whichfile=which;
        if (orient==1) {
          if (which=='X') whichfile='Y';    /* Rotation symmetry: calculate per-per from current data   */
          if (which=='Y') whichfile='X';    /* CalculateE is called from calculator with 'Y' polarization */
          incPol=incPolper;                 /* we now just assume that we have the x-z plane as the scatering plane, */
          incPolper=incPolpar;              /* rotating in the negative x-direction. This mimics the real case of */
          incPolpar=incPol;                 /* X polarization with the y-z plane as scattering plane */
          MultScal(-1,incPolpar,incPolpar); /* Then IncPolY -> -IncPolX; incPolX -> IncPolY */
        }
        if ((symY && whichfile=='Y') || (symX && whichfile=='X') || orient_avg)
          nmax=nTheta;
        /* if not enough symmetry, calculate for +- theta */
        else nmax=2*(nTheta-1);
        /* correct beginning of Epar array */
        Eper=Epar+nmax;

        for (i = 0; i < nmax; ++i) {
          theta = i * dtheta;
          co=cos(theta);
          si=sin(theta);
          LinComb(prop,incPolpar,co,si,robserver);  /* robserver = co*prop + si*incPolpar; */

          calc_field(ebuff,robserver);

          /* convert to (l,r) frame */
          crDotProd(ebuff,incPolper,Eper[i]); 	 /* Eper[i]=Esca.incPolper */
          LinComb(prop,incPolpar,-si,co,epar);         /* epar=-si*prop+co*incPolpar */
          crDotProd(ebuff,epar,Epar[i]); 	         /* Epar[i]=Esca.epar */
        } /*  end for i */

        /* ACCUMULATE EPAR AND EPER TO ROOT AND SUMMATE */
        D("accumulating Epar and Eper");
        /* accumulate only on processor 0 !, done in one operation */
        tstart2=clock();
        accumulate((double *)Epar,4*nmax,Eparper_buffer);
        Timing_comm_EField = clock() - tstart2;
        D("done accumulating");

        if (orient_avg && ringid==ROOT) {
          if (whichfile=='X') index=0;
          else if (whichfile=='Y') index=3;
          memcpy(&ampl_alpha[nTheta*(index+4*k_or)],Eper,nTheta*sizeof(doublecomplex));
          if (whichfile=='X') index=2;
          else if (whichfile=='Y') index=1;
          memcpy(&ampl_alpha[nTheta*(index+4*k_or)],Epar,nTheta*sizeof(doublecomplex));
        }

        Timing_EFieldPlane = clock() - tstart;
        Timing_calc_EField = Timing_EFieldPlane - Timing_comm_EField;
        Timing_EField += Timing_EFieldPlane;
        TotalEFieldPlane++;

        if (ringid==ROOT && !orient_avg) {		 /* WRITE RESULTS TO FILE */
          tiostart = clock ();
          switch (whichfile) {
          case 'X' :
            strcpy(stringbuffer,directory);
            strcat(stringbuffer,"/per-par");
            if(xzplane==true) strcat(stringbuffer,"-yz");
            filepar = fopen (stringbuffer, "w");
            strcpy(stringbuffer,directory);
            strcat(stringbuffer,"/per-per");
            if(xzplane==true) strcat(stringbuffer,"-yz");
            fileper = fopen (stringbuffer, "w");
            if (filepar==NULL || fileper==NULL)
              LogError(EC_ERROR,ONE,POSIT,"Could not open fileper or filepar");
            break;
          case 'Y' :
            strcpy(stringbuffer,directory);
            strcat(stringbuffer,"/par-par");
            if(xzplane==true) strcat(stringbuffer,"-yz");
            filepar = fopen (stringbuffer, "w");
            strcpy(stringbuffer,directory);
            strcat(stringbuffer,"/par-per");
            if(xzplane==true) strcat(stringbuffer,"-yz");
            fileper = fopen (stringbuffer, "w");
            if (filepar==NULL || fileper==NULL)
              LogError(EC_ERROR,ONE,POSIT,"Could not open fileper or filepar");
            break;
          }

  	  D("writing to files ...\n");
  	  for (i = 0; i < nmax; ++i) {
  	    fprintf (filepar,"%.2f % .7E     % .7E\n",
  	        180/PI*i*dtheta,Epar[i][re], Epar[i][im]);
  	    fflush(filepar);
  	    fprintf(fileper,"%.2f % .7E     % .7E\n",
  		 180/PI*i*dtheta,Eper[i][re], Eper[i][im]);
  	    fflush(fileper);
  	  }
  	  fclose (filepar);
  	  fclose (fileper);
  	  Timing_FileIO += clock() - tiostart;
        }
      } /* end of orient loop */
    } /* end of alpha loop */
  }  /* end of yzplane */

  if(xzplane==true) { /*also calculate fields in xz-plane */
    /* this code was changed and debugged in dec. 2003 */
    /* now it works good */

    /* !!!!! works only for defualt incidence, has not been parallelized yet*/
    if (type==NORMAL) Norient=1; else Norient=2; /* number of orientations */
                                                  /* Norient is not working !!! */
    dtheta = PI / ((double)(nTheta-1));

    for(orient=0;orient<Norient;orient++) {
      /* in case of Rotation symmetry */
      tstart = clock ();

      if (orient==0) whichfile=which;
      if (orient==1 && which=='X') whichfile='Y'; /* swap X and Y */
      if (orient==1 && which=='Y') whichfile='X';

      if ((symY==true && whichfile=='Y') || (symX==true && whichfile=='X'))
        nmax=nTheta;
      /* if not enough symmetry, calculate for +- theta */
      else nmax=2*(nTheta-1);
      /* correct beginning of Epar array */
      Eper=Epar+nmax;

      for (i = 0; i < nmax; ++i) {
        theta = i * dtheta;

        if (orient==0) { /* normal procedure */
          robserver[0] = sin (theta);
          robserver[1] = 0.0;
          robserver[2] = cos (theta);
        }
        else { /* Rotation symmetry: calculate par-par from current data */
          robserver[0] = 0.0;
          robserver[1] = sin (theta);
          robserver[2] = cos (theta);
        }

        calc_field(ebuff,robserver);

        /* convert to (l,r) frame */
        if (orient==0) {
          if(which=='X') {
          Eper[i][re] = -ebuff[1][re];
          Eper[i][im] = -ebuff[1][im];
          Epar[i][re] = cos (theta) * ebuff[0][re] - sin (theta) * ebuff[2][re];
          Epar[i][im] = cos (theta) * ebuff[0][im] - sin (theta) * ebuff[2][im];
          }
          else {/*which == 'Y', so all fields must be phase shifted 180 degrees */
	    Eper[i][re] = ebuff[1][re];
	    Eper[i][im] = ebuff[1][im];
	    Epar[i][re] = -(cos (theta) * ebuff[0][re] - sin (theta) * ebuff[2][re]);
	    Epar[i][im] = -(cos (theta) * ebuff[0][im] - sin (theta) * ebuff[2][im]);
	  }
	}
	else {
	  Eper[i][re]=ebuff[0][re];
	  Eper[i][im]=ebuff[0][im];
	  /* these two lines are not checked for bugs */
	  Epar[i][re] = cos (theta) * ebuff[1][re] - sin (theta) * ebuff[2][re];
	  Epar[i][im] = cos (theta) * ebuff[1][im] - sin (theta) * ebuff[2][im];
	}
      } /*  end for i */

      /* ACCUMULATE EPAR AND EPER TO ROOT AND SUMMATE */
      D("accumulating Epar and Eper");
      /* accumulate only on processor 0 !, done in one operation */
      accumulate((double *)Epar,4*nmax,Eparper_buffer);
      D("done accumulating");

      Timing_EField += clock() - tstart;
      TotalEFieldPlane++;

      if (ringid==ROOT) {		/* WRITE RESULTS TO FILE */
        tiostart = clock ();
        switch (whichfile) {
        case 'X' :
          strcpy(stringbuffer,directory);
          strcat(stringbuffer,"/par-par-xz");
          filepar = fopen (stringbuffer, "w");
          strcpy(stringbuffer,directory);
          strcat(stringbuffer,"/par-per-xz");
          fileper = fopen (stringbuffer, "w");
          if (filepar==NULL || fileper==NULL)
            LogError(EC_ERROR,ONE,POSIT,"Could not open to fileper or filepar");
          break;
        case 'Y' :
          strcpy(stringbuffer,directory);
          strcat(stringbuffer,"/per-par-xz");
          filepar = fopen (stringbuffer, "w");
          strcpy(stringbuffer,directory);
          strcat(stringbuffer,"/per-per-xz");
          fileper = fopen (stringbuffer, "w");
          if (filepar==NULL || fileper==NULL)
            LogError(EC_ERROR,ONE,POSIT,"Could not open fileper or filepar");
          break;
        }

        D("writing to files ...\n");
        for (i = 0; i < nmax; ++i) {
          fprintz (filepar,"%.2f % .7E     % .7E\n",
      	     180/PI*i*dtheta,Epar[i][re], Epar[i][im]);
          fflush(filepar);
          fprintz (fileper,"%.2f % .7E     % .7E\n",
      	     180/PI*i*dtheta,Eper[i][re], Eper[i][im]);
          fflush(fileper);
        }
        fclose (filepar);
        fclose (fileper);
        tiostop = clock ();
        Timing_FileIO += tiostop - tiostart;
      }
    } /* end of orient loop */
  } /* end of xzplane */

  if (all_dir) calc_alldir(which);
    /* Calculate the scattered field for the total space-angle
     * in discretized phi and theta */

  if (calc_Cext || calc_Cabs || calc_Csca || calc_asym || calc_mat_force) {

    D("Calculation of cross sections started");
    tstart = clock();

    strcpy(stringbuffer,directory);
    if (which == 'X') {
      strcat(stringbuffer,"/CrossSec-X");
      incPol=incPolX;
    }
    if (which == 'Y') {
      strcat(stringbuffer,"/CrossSec-Y");
      incPol=incPolY;
    }

    if (calc_Cext) Cext = Ext_cross(incPol);
    if (calc_Cabs) Cabs = Abs_cross();
    D("Cext and Cabs calculated");

    if (orient_avg) {
      if (ringid==ROOT) {
        if (which == 'Y') {        /* assumed that first call of CalculateE is with 'Y' flag */
          muel_alpha[-2]=Cext;
          muel_alpha[-1]=Cabs;
        }
        else if (which == 'X') {
          muel_alpha[-2]=(muel_alpha[-2]+Cext)/2;
          muel_alpha[-1]=(muel_alpha[-1]+Cabs)/2;
        }
      }
    }
    else {  /* not orient_avg */
      if (ringid==ROOT) {
        if ((CCfile=fopen(stringbuffer,"w"))==NULL)
          LogError(EC_ERROR,ONE,POSIT,"Could not write to file '%s'",stringbuffer);
        /* calculate volume_equivalent radius and cross-section */
        a_eq = pow((0.75/PI)*nvoid_Ndip,.333333333333)*gridspace;
        G = PI*a_eq*a_eq;
        fprintf(CCfile,"x=%.8g\n\n",WaveNum*a_eq);
        if (calc_Cext) {
          printf("Cext\t= %12.8lg\nQext\t= %12.8lg\n",Cext,Cext/G);
          fprintf(CCfile,"Cext\t= %12.8lg\nQext\t= %12.8lg\n",Cext,Cext/G);
        }
        if (calc_Cabs) {
          printf("Cabs\t= %12.8lg\nQabs\t= %12.8lg\n",Cabs,Cabs/G);
          fprintf(CCfile,"Cabs\t= %12.8lg\nQabs\t= %12.8lg\n",Cabs,Cabs/G);
        }
        if (calc_Csca) {
          fprintf(CCfile,"\nIntegration\n");
          printf("int Csca\n");
          Csca=Sca_cross();
          printf("Csca\t= %12.8lg\nQsca\t= %12.8lg\t  (integration)\n\n",
          Csca,Csca/G);
          fprintf(CCfile,"Csca\t= %12.8lg\nQsca\t= %12.8lg\n",Csca,Csca/G);
        }
        if (calc_vec) {
          printf("\n\nint asym-x\n");
          Asym_parm_x(dummy);
          printf("g.Csca-x\t= %12.8lg\n",dummy[0]);
          if (calc_asym) printf("g-x\t\t= %12.8lg\n",g[0]=dummy[0]/Csca);
          printf("\n\nint asym-y\n");
          Asym_parm_y(dummy+1);
          printf("g.Csca-y\t= %12.8lg\n",dummy[1]);
          if (calc_asym) printf("g-y\t\t= %12.8lg\n",g[1]=dummy[1]/Csca);
          printf("\n\nint asym-z\n");
          Asym_parm_z(dummy+2);
          printf("g.Csca-z\t= %12.8lg\n",dummy[2]);
          if (calc_asym) printf("g-z\t\t= %12.8lg\n",g[2]=dummy[2]/Csca);

          fprintf(CCfile,"Csca.g\t=(%12.8lg,%12.8lg,%12.8lg)\n",dummy[0],dummy[1],dummy[2]);
          if (calc_asym) fprintf(CCfile,"g\t=(%12.8lg,%12.8lg,%12.8lg)\n",g[0],g[1],g[2]);
        }
      } /* end of ROOT */
      if (calc_mat_force) {
        if ((Fsca = (double *) calloc(3*local_nvoid_Ndip,sizeof(double))) == NULL)
	  LogError(EC_ERROR,ALL,POSIT,"Could not malloc Fsca");
        if ((Finc = (double *) calloc(3*local_nvoid_Ndip,sizeof(double))) == NULL)
          LogError(EC_ERROR,ALL,POSIT,"Could not malloc Finc");
        if ((Frp = (double *) calloc(3*local_nvoid_Ndip,sizeof(double))) == NULL)
          LogError(EC_ERROR,ALL,POSIT,"Could not malloc Frp");

        printz("Calculating the force per dipole\n");

        /* Calculate forces */
        Frp_mat(Fsca_tot,Fsca,Finc_tot,Finc,Frp_tot,Frp);

        /* Write Cross-Sections and Efficiencies to file */
        Cnorm = 8*PI;
        Qnorm = 8*PI/G;
        printz("\nMatrix\n"\
               "Cext\t= %12.8lg\nQext\t= %12.8lg\n"\
               "Csca.g\t= (%12.8lg,%12.8lg,%12.8lg)\n"\
               "Cpr\t= (%12.8lg,%12.8lg,%12.8lg)\n"\
               "Qpr\t= (%12.8lg,%12.8lg,%12.8lg)\n",
               Cnorm*Finc_tot[2],Qnorm*Finc_tot[2],
               -Cnorm*Fsca_tot[0],-Cnorm*Fsca_tot[1],-Cnorm*Fsca_tot[2],
               Cnorm*Frp_tot[0],Cnorm*Frp_tot[1],Cnorm*Frp_tot[2],
               Qnorm*Frp_tot[0],Qnorm*Frp_tot[1],Qnorm*Frp_tot[2]);
        fprintz(CCfile,"\nMatrix\n"\
                "Cext\t= %12.8lg\nQext\t= %12.8lg\n"\
                "Csca.g\t= (%12.8lg,%12.8lg,%12.8lg)\n"\
                "Cpr\t= (%12.8lg,%12.8lg,%12.8lg)\n"\
                "Qpr\t= (%12.8lg,%12.8lg,%12.8lg)\n",
                Cnorm*Finc_tot[2],Qnorm*Finc_tot[2],
                -Cnorm*Fsca_tot[0],-Cnorm*Fsca_tot[1],-Cnorm*Fsca_tot[2],
                Cnorm*Frp_tot[0],Cnorm*Frp_tot[1],Cnorm*Frp_tot[2],
                Qnorm*Frp_tot[0],Qnorm*Frp_tot[1],Qnorm*Frp_tot[2]);

        if (store_force) {
        /* Write Radiation pressure per dipole to file */
          strcpy(stringbuffer,directory);
          if (which == 'X') strcat(stringbuffer,"/VisFrp-X.dat");
          else if (which == 'Y') strcat(stringbuffer,"/VisFrp-Y.dat");

          if ((VisFrp=fopen(stringbuffer,"w"))==NULL)
            LogError(EC_ERROR,ONE,POSIT,"Could not write to file '%s'",stringbuffer);

          fprintf(VisFrp,"#sphere\t\t\tx=%.8lg\tm=%.8lg%+.8lgi\n"\
                  "#number of dipoles\t%d\n"\
                  "#Forces per dipole\n"\
                  "#r.x\t\tr.y\t\tr.z\t\tF.x\t\tF.y\t\tF.z\n",
                  WaveNum*a_eq,ref_index[0][re],ref_index[0][im],
                  nvoid_Ndip);
          for (j=0;j<local_nvoid_Ndip;++j)
            fprintf(VisFrp,
                    "%10.8g\t%10.8g\t%10.8g\t"\
                    "%10.8g\t%10.8g\t%10.8g\n",
                    DipoleCoord[3*j],DipoleCoord[3*j+1],
                    DipoleCoord[3*j+2],
                    Frp[3*j],Frp[3*j+1],Frp[3*j+2]);
          fclose(VisFrp);
        }

        free(Fsca);
        free(Finc);
        free(Frp);
      }
      fclosez(CCfile);
    }
    D("Calculation of cross sections complete");
    Timing_ScatQuan += clock() - tstart;
  }
  if (all_dir) finish_int();

  if (store_int_field) {
    /* Write internal field on each dipole to file */
    /* all processors should write the internal field, stored in */
    /* the vector x to file. Files get the name IntfieldY-procid */
    /* or IntfieldXr-procid */
    /* saves actual internal fields (not exciting as in previous versions) */
    tstart=clock();
    /* calculate internal fields */
    V=gridspace*gridspace*gridspace;
    for (i=0;i<Nmat;i++) {
      /* hi_inv=1/(V*hi)=4*PI/(V(m^2-1)) */
      cSquare(ref_index[i],hi);
      hi[re]-=1;
      cMultReal(V,hi,hi);
      cInv(hi,hi_inv[i]);
      cMultReal(4*PI,hi_inv[i],hi_inv[i]);
    }
    /* open files */
    if (which=='X') sprintf(stringbuffer,"%s%s%i",directory,"/IntFieldX_",ringid);
    else if (which=='Y') sprintf(stringbuffer,"%s%s%i",directory,"/IntFieldY_",ringid);
    if ((Intfldpntr=fopen(stringbuffer,"w"))==NULL)
      LogError(EC_ERROR,ONE,POSIT,"Could not write to file '%s'",stringbuffer);

    if (ringid==ROOT)
      fprintf(Intfldpntr,
        "x\t\ty\t\tz\t\t|E|^2\t\tEx.r\t\tEx.i\t\tEy.r\t\tEy.i\t\tEz.r\t\tEz.i\t\t\n\n");
    /* saves fields to file */
    for (i=0;i<local_nvoid_Ndip;++i) {
      mat=material[i];
      for (j=0;j<3;j++) cMult(hi_inv[mat],p[3*i+j],fld[j]);   /* field=P/(V*hi) */
      fprintf(Intfldpntr,
        "%10.8g\t%10.8g\t%10.8g\t%10.8g\t%10.8g\t%10.8g\t%10.8g\t%10.8g\t%10.8g\t%10.8g\n",
        DipoleCoord[3*i],DipoleCoord[3*i+1], DipoleCoord[3*i+2],
        cvNorm2(fld),
        fld[0][re],fld[0][im],fld[1][re],fld[1][im],fld[2][re],fld[2][im]);
    }
    Timing_FileIO += clock() - tstart;
  }
}
