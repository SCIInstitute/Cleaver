//------------------------------
//
// This header file is necessary due
// to the inflexiblity of the Stellar
// build design.
//
// Jonathan Bronson
//

#ifndef STELLAR_H
#define STELLAR_H

#include "Starbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* types of quality measures that may be used */
enum tetqualitymetrics
{
    QUALMINSINE,
    QUALRADIUSRATIO,
    QUALVLRMS3RATIO,
    QUALMEANSINE,
    QUALMINSINEANDEDGERATIO,
    QUALWARPEDMINSINE,
    QUALMINANGLE,
    QUALMAXANGLE
};

/* types of anisotropic sizing fields */
enum tensorfields
{
    NOTENSOR,
    STRETCHX,
    STRETCHY,
    SINK,
    SWIRL,
    CENTER,
    PERIMETER,
    RIGHT,
    SINE,
    STRAIN,
    CUSTOM     // added by J.R.B.
};


/* for printing color in output to the screen */
#define RESET		0
#define BRIGHT 		1
#define DIM         2
#define UNDERLINE 	3
#define BLINK		4
#define REVERSE		7
#define HIDDEN		8

#define BLACK 		0
#define RED		    1
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define	WHITE		7


/* a really big floating point number */
#define HUGEFLOAT 1.0e100


/* vertex types */
#define INPUTVERTEX 0
#define FIXEDVERTEX 1
#define SEGMENTVERTEX 2
#define FACETVERTEX 3
#define FREEVERTEX 4
#define UNDEADVERTEX 15
/* new kind of vertex to identify the ones I've put in */
#define INSERTEDVERTEX 4

/* types of improvement passes */
#define SMOOTHPASS 0
#define TOPOPASS 1
#define CONTRACTPASS 2
#define INSERTPASS 3
#define DESPERATEPASS 4
#define DEFORMPASS 5

/* types of local improvement passes */
#define SMOOTHPASSLOCAL 6
#define TOPOPASSLOCAL 7
#define CONTRACTPASSLOCAL 8
#define INSERTPASSLOCAL 9

/* size control pass */
#define SIZECONTROLPASS 10
#define CONTRACTALLPASS 11

/* number of quality measures */
#define NUMQUALMEASURES 6

/* number of "thresholded" means used to approximate mesh quality */
#define NUMMEANTHRESHOLDS 7


/* edge cases */
#define NUMEDGECASES 10
#define NOEDGECASE 0
#define FREEFREEEDGE 1
#define FREEFACETEDGE 2
#define FREESEGMENTEDGE 3
#define FREEFIXEDEDGE 4
#define FACETFACETEDGE 5
#define FACETSEGMENTEDGE 6
#define FACETFIXEDEDGE 7
#define SEGMENTSEGMENTEDGE 8
#define SEGMENTFIXEDEDGE 9
#define FIXEDFIXEDEDGE 10


/* number of passes without improvement before static improvement quits */
#define STATICMAXPASSES 3
/* number of desperate insertion passes that can ever be attempted */
#define DESPERATEMAXPASSES 3

/* maximum size of stuff for cavity drilling */
#define MAXCAVITYFACES 10000
#define MAXCAVITYTETS 10000

/* deepest level a tet can be */
#define MAXCAVDEPTH 1000

/*****************************************************************************/
/*  Topological improvement options                                          */
/*****************************************************************************/

/* number of tets to allow in a ring around an edge to be removed */
#define MAXRINGTETS 70
#define MAXRINGTETS2 50

/* maximum number of tets in sandwich set replacement during edge removal */
#define MAXNEWTETS 150

/* maximum number of faces in tree for multi-face removal */
#define MAXFACETREESIZE 50

/* minimum quality to allow a 4-verts-on-boundary tet to be created
   by a topological improvement operation */
#define MIN4BOUNDQUAL SINE1


/*****************************************************************************/
/*                                                                           */
/*  Data structures                                                          */
/*                                                                           */
/*****************************************************************************/

/* store mappings from tags to vertex types */
struct vertextype
{
    int kind;        /* the kind of vector this is (FREEVERTEX, FACETVERTEX, etc) */
    starreal vec[3]; /* a vector associated with the vertex. for FACETVERTEX,
                        this is the normal to the plane that the vertex can move in.
                        for SEGMENTVERTEX, this is a vector in the direction of the
                        segment. */
};

/* structure to hold all the options for improvement */
struct improvebehavior
{
    /* Quality measure */
    int qualmeasure;             /* quality measure used */
    starreal sinewarpfactor;     /* warp obtuse sines by this factor */

    /* Quadric smoothing options */
    int usequadrics;             /* incorporate quadric error into the objective function */
    starreal quadricoffset;      /* quality to start every quadric at */
    starreal quadricscale;       /* factor to scale quadric by */

    /* Smoothing options */
    int nonsmooth;               /* enable non-smooth optimization-based smoothing */
    int facetsmooth;             /* enable smoothing of facet vertices */
    int segmentsmooth;           /* enable smoothing of segment vertices */
    int fixedsmooth;             /* enable smoothing of fixed vertices */

    /* Topological options */
    int edgeremoval;             /* enable edge removal */
    int edgecontraction;         /* enable edge contraction */
    int boundedgeremoval;        /* enable boundary edge removal */
    int singlefaceremoval;       /* enable single face removal (2-3 flips) */
    int multifaceremoval;        /* enable multi face removal */
    int flip22;                  /* enable 2-2 flips */
    int jflips;                  /* use Jonathan's faster flip routines */

    /* Insertion options */
    int enableinsert;            /* global enable of insertion */
    starreal insertthreshold;    /* percent worst tets */
    int insertbody;              /* enable body vertex insertion */
    int insertfacet;             /* enable facet insertion */
    int insertsegment;           /* enablem segment insertion */
    int cavityconsiderdeleted;   /* consider enlarging cavity for deleted tets? */
    int cavdepthlimit;           /* only allow initial cavity to includes tets this deep */

    /* anisotropic meshing options */
    int anisotropic;              /* globally enable space warping with deformation tensor */
    int tensor;                   /* which scaling tensor field to use */
    int tensorb;                  /* second tensor, to blend with the first one */
    starreal tensorblend;         /* between 0 and 1, how much of anisotropy to use */

    /* sizing options */
    int sizing;                  /* globally enable mesh element size control */
    int sizingpass;              /* enable or disable initial edge length control */
    starreal targetedgelength;   /* edge length of the ideal edge for this mesh */
    starreal longerfactor;       /* factor by which an edge can be longer */
    starreal shorterfactor;      /* factor by which an edge can be shorter */

    /* Dynamic improvement options */
    starreal dynminqual;         /* minimum quality demanded for dynamic improvement. */
    starreal dynimproveto;       /* after minimum quality is reached, improve to at least this level */
    int deformtype;              /* which fake deformation to use */
    int dynimprove;              /* perform dynamic improvement with fake deformation? */

    /* thresholds */
    starreal minstepimprovement; /* demand at least this much improvement in the mean per step */
    starreal mininsertionimprovement; /* demand in improvement for insertion */
    starreal maxinsertquality[NUMQUALMEASURES];   /* never attempt insertion in a tet better than this */

    /* improvement limits */
    starreal goalanglemin;       /* stop improvement if smallest angle reaches this threshold */
    starreal goalanglemax;       /* stop improvement if largest angle reaches this threshold */

    /* quality file output */
    int minsineout;              /* en/disable .minsine file output */
    int minangout;               /* en/disable .minang file output */
    int maxangout;               /* en/disable .maxang file output */
    int vlrmsout;                /* en/disable .vlrms file output */
    int nrrout;                 /* en/disable .rnrr file output */

    /* output file name prefix */
    char fileprefix[100];

    /* enable animation */
    int animate;
    /* for animation, only output .stats */
    int timeseries;

    /* verbosity */
    int verbosity;
    int usecolor;

    /* miscellaneous */
    int outputandquit;           /* just produce all output files for unchanged mesh */
};


/* structure to hold global improvement statistics */
struct improvestats
{
    /* smoothing stats */
    int nonsmoothattempts;
    int nonsmoothsuccesses;
    int freesmoothattempts;
    int freesmoothsuccesses;
    int facetsmoothattempts;
    int facetsmoothsuccesses;
    int segmentsmoothattempts;
    int segmentsmoothsuccesses;
    int fixedsmoothattempts;
    int fixedsmoothsuccesses;

    /* topological stats */
    int edgeremovals;
    int boundaryedgeremovals;
    int edgeremovalattempts;
    int boundaryedgeremovalattempts;
    int ringsizesuccess[MAXRINGTETS];
    int ringsizeattempts[MAXRINGTETS];
    int faceremovals;
    int faceremovalattempts;
    int facesizesuccess[MAXFACETREESIZE];
    int facesizeattempts[MAXFACETREESIZE];
    int flip22attempts;
    int flip22successes;
    int edgecontractionattempts;
    int edgecontractions;
    int edgecontractcaseatt[NUMEDGECASES+1];
    int edgecontractcasesuc[NUMEDGECASES+1];
    int edgecontractringatt[MAXRINGTETS];
    int edgecontractringsuc[MAXRINGTETS];

    /* insertion stats */
    int bodyinsertattempts;
    int bodyinsertsuccesses;
    int facetinsertattempts;
    int facetinsertsuccesses;
    int segmentinsertattempts;
    int segmentinsertsuccesses;
    int maxcavitysizes[MAXCAVITYTETS];
    int finalcavitysizes[MAXCAVITYTETS];
    int biggestcavdepths[MAXCAVDEPTH];
    int lexmaxcavdepths[MAXCAVDEPTH];

    /* timing stats */
#ifndef NO_TIMER
    struct timeval starttime;
#endif /* not NO_TIMER */
    int totalmsec;
    int smoothmsec;
    int topomsec;
    int contractmsec;
    int insertmsec;
    int smoothlocalmsec;
    int topolocalmsec;
    int insertlocalmsec;
    int contractlocalmsec;
    int biggestcavityusec;
    int finalcavityusec;
    int cavityimproveusec;

    /* general stats */
    int startnumtets;
    int finishnumtets;
    int startnumverts;
    int finishnumverts;
    starreal dynchangedvol;

    /* quality stats */
    starreal finishworstqual;
    starreal startworstqual;
    starreal startminangle;
    starreal startmaxangle;
    starreal startmeanquals[NUMMEANTHRESHOLDS];
    starreal finishmeanquals[NUMMEANTHRESHOLDS];
};

struct arraypoolstack
{
    struct arraypool pool; /* the array pool that makes up the stack */
    long top;     /* the index of the top element in the array */
    long maxtop;  /* the maximum size the stack has ever been */
};

/* pre-improvement initialization code */
void improveinit(struct tetcomplex *mesh,
                 struct proxipool *vertexpool,
                 struct arraypoolstack *tetstack,
                 struct behavior *behave,
                 struct inputs *in,
                 int argc,
                 char **argv,
                 starreal bestmeans[NUMMEANTHRESHOLDS]);


/* clean up after mesh improvement */
void improvedeinit(struct tetcomplex *mesh,
                   struct proxipool *vertexpool,
                   struct arraypoolstack *tetstack,
                   struct behavior *behave,
                   struct inputs *in,
                   int argc,
                   char **argv);

/* top-level function to perform static mesh improvement */
void staticimprove(struct behavior *behave,
                   struct inputs *in,
                   struct proxipool *vertexpool,
                   struct tetcomplex *mesh,
                   int argc,
                   char **argv);



/* run a pass (smoothing, topo, insertion). return true
   if we have reached the desired quality */
bool pass(int passtype,
          struct tetcomplex* mesh,
          struct arraypoolstack* tetstack,
          starreal threshold,
          bool *minsuccess,
          bool *meansuccess,
          int passnum,
          starreal bestmeans[],
          struct behavior *behave,
          struct inputs *in,
          struct proxipool *vertexpool,
          int argc,
          char **argv);

void parseimprovecommandline(int argc,
                             char **argv,
                             struct improvebehavior *b);

/* print a report on edge lengths in the mesh */
void sizereport(struct tetcomplex *mesh);


/* split and collapse edges until all tetrahedra are roughly the same size */
int sizecontrol(struct tetcomplex *mesh,
                 struct behavior *behave,
                 struct inputs *in,
                 struct proxipool *vertexpool,
                 int argc,
                 char **argv);

/* given an input mesh, find the worst "input" angle.
   that is, find the smallest angle between two faces
   of the boundary */
starreal worstinputangle(struct tetcomplex *mesh);

/* gather some information about the worst tets in the mesh */
/* according to the given quality measure, report information
   about all the tets within degreesfromworst of the worst tet */
void worsttetreport(struct tetcomplex *mesh,
                    int qualmeasure,
                    starreal degreesfromworst);

void fillstackqual(struct tetcomplex *mesh,
                   struct arraypoolstack *stack,
                   int qualmeasure,
                   starreal threshold,
                   starreal *meanqual,
                   starreal *minqual);

int countverts(struct proxipool *vertpool);
int counttets(struct tetcomplex *mesh);

void printstats(tetcomplex *mesh);

void checkquadrics(tetcomplex *mesh);

void improvestatistics(struct behavior *behave,
                       struct tetcomplex *plex,
                       bool anisotropic);

// Convenience Functions
void textcolor(int attr, int fg, int bg);
int mytetcomplexconsistency(struct tetcomplex *plex);


/* free the stack data structure */
void stackdeinit(struct arraypoolstack *stack);

/* print out the entire mesh. includes node positions,
   tet node values, and tet quality values */
void outputqualmesh(struct behavior *b,
                    struct inputs *in,
                    struct proxipool *vertexpool,
                    struct tetcomplex *mesh,
                    int argc,
                    char **argv,
                    int passnum,
                    int passtype,
                    int passstartid,
                    int qualmeasure);

/* renumber vertices to include the inserted ones */
void myoutputnumbervertices(struct behavior *behave,
                          struct inputs *in,
                          struct proxipool *pool);

/* customized vertex output TODO: this is mostly copied from Jonathan's
   output routines */
void myoutputvertices(struct behavior *behave,
                    struct inputs *in,
                    struct proxipool *pool,
                    int argc,
                    char **argv);

// Main Stellar Driver
int mainfunc(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif // STELLAR_H
