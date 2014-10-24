#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#pragma warning (disable:4996)

#define AIR_DENSITY                 1.165      // SI Units
#define AIR_DYNAMIC_VISCOSITY       1.868E-5   // SI Units
#define AIR_KINEMATIC_VISCOSITY     (AIR_DYNAMIC_VISCOSITY/AIR_DENSITY)

#define PI  3.14159265358979323846

#define G   9.806  // gravitational acceleration in SI units

#define MAX_NAME  20      // for club names
#define MAX_SIZE  5000    // maximum static array size

#define NUM_CLUBS 10       // see Table 1 in the lab3 document

#define TASK1_X_TOL_YARDS        2.0   // for finding timesteps for each integration method
#define Z_TOL                    1.0e-5 // ball landing altitude tolerance

#define SPIN_DECAY_CONSTANT   0.05

#define GOLF_BALL_DIAMETER 0.04267  // SI units
#define GOLF_BALL_MASS     0.0456   // SI units

#define IDRIVER   0   // indexes for arrays of clubs
#define I3WOOD    1
#define I3IRON    2
#define I4IRON    3
#define I5IRON    4
#define I6IRON    5
#define I7IRON    6
#define I8IRON    7
#define I9IRON    8
#define IWEDGE    9

#define IEULER       0  // indexes for different integration methods
#define ICROMER      1
#define IRICHARDSON  2

#define YARDS_TO_METERS(X)    ((X)*0.9144)  // various conversion macros
#define METERS_TO_YARDS(X)    ((X)/0.9144)
#define RAD_TO_DEG(X)         ((X)*180.0/PI)
#define DEG_TO_RAD(X)         ((X)*PI/180.0)
#define MPH_TO_MPS(X)         ((X)*0.44704)
#define MPS_TO_MPH(X)         ((X)/0.44704)
#define RPM_TO_RADPS(X)       ((X)*2.0*PI/60.0)
#define RADPS_TO_RPM(X)       ((X)*60.0/(2.0*PI))

// structures you can use in the program
typedef struct POINT_XZ  // for arrays of trajectory data (2D in xz plane)
{
   double x,z;  // careful about units.  Suggest these are in meters
}
POINT_XZ;

typedef struct TRACKMAN_DATA  // you must use this to get trackman data
{
   // see Table 1 in the lab3 document
   char name[MAX_NAME+1]; // +1 for NUL
   double ballSpeedMPH,tailWindMPH,launchAngleDeg,
          spinRPM,zMaxYards,impactAngleDeg,xMaxYards;
}
TRACKMAN_DATA;

typedef struct TRAJECTORY_DATA  // returned from function that checks impact spot
{
   double zMaxYards,xMaxYards,impactAngleDeg;
}
TRAJECTORY_DATA;

//---------------------------------------------------------------------------------------
//function prototypes

void closeAndExit();
void getTrackmanData(TRACKMAN_DATA *);
void task1(TRACKMAN_DATA *);
void task2(TRACKMAN_DATA *);
void task3(TRACKMAN_DATA *);
TRAJECTORY_DATA getTrajectoryData(POINT_XZ*,int);
double getCd(double,double);
double getCl(double);
double getSpin(double,double);

//---------------------------------------------------------------------------------------

// the one and only global variable (ok for log file)
FILE *flog;

//---------------------------------------------------------------------------------------

int main()
{
   TRACKMAN_DATA tmd[NUM_CLUBS];

   flog=fopen("log.txt","w");
   if(flog==NULL)
   {
      printf("Sorry but I can't open a log file for writing.\n");
      closeAndExit();
   }

   getTrackmanData(tmd);
   task1(tmd);
   task2(tmd);
   task3(tmd);

   closeAndExit();
   return 0;
}

//---------------------------------------------------------------------------------------

void closeAndExit()
{
   printf("\n\nPress ENTER to end this program.");
   getchar();
   if(flog!=NULL) fclose(flog);
   exit(0);
}

//---------------------------------------------------------------------------------------

void getTrackmanData(TRACKMAN_DATA *tmd)
{
   int i;
   char *name[NUM_CLUBS]={"Driver","3-Wood","3-Iron","4-Iron","5-Iron",
                          "6-Iron","7-Iron","8-Iron","9-Iron","Wedge"};
   double ballSpeedMPH[NUM_CLUBS]={167.,158.,142.,137.,132.,127.,120.,115.,109.,102.};
   double tailWindMPH[NUM_CLUBS]={0.,-3.,0.,-1.,0.,5.,5.,4.,-2.,0.};
   double launchAngleDeg[NUM_CLUBS]={10.9,9.2,10.4,11.0,12.1,14.1,16.3,18.1,20.4,24.2};
   double spinRPM[NUM_CLUBS]={2690.,3650.,4630.,4840.,5360.,6230.,7100.,8000.,8650.,9300.};
   double zMmaxYards[NUM_CLUBS]={26.,25.,24.,24.,25.,26.,28.,29.,31.,31.};
   double impactAngleDeg[NUM_CLUBS]={34.,36.,37.,38.,40.,40.,43.,46.,53.,53};
   double xMmaxYards[NUM_CLUBS]={251.,232.,209.,199.,191.,186.,171.,157.,135.,122.};

   for(i=0;i<NUM_CLUBS;i++)
   {
      strcpy(tmd[i].name,name[i]);
      tmd[i].ballSpeedMPH=ballSpeedMPH[i];
      tmd[i].tailWindMPH=tailWindMPH[i];
      tmd[i].launchAngleDeg=launchAngleDeg[i];
      tmd[i].spinRPM=spinRPM[i];
      tmd[i].impactAngleDeg=impactAngleDeg[i];
      tmd[i].xMaxYards=xMmaxYards[i];
      tmd[i].zMaxYards=zMmaxYards[i];
   }
}

//---------------------------------------------------------------------------------------

void task1(TRACKMAN_DATA *tmd)
{
   //example of how to use the trackman data array and the club indexes
   int n;
   for(n=0;n<NUM_CLUBS;n++)
   {
      printf("The %-7s distance is %.1lf yards\n",tmd[n].name,tmd[n].xMaxYards);
      if(n==I3WOOD || n==I9IRON)
      {
         printf("-----------------------------------\n");
      }
   }
}

//---------------------------------------------------------------------------------------

void task2(TRACKMAN_DATA *tmd)
{
}

//---------------------------------------------------------------------------------------

void task3(TRACKMAN_DATA *tmd)
{
}

//---------------------------------------------------------------------------------------
// This function takes in a POINT_XZ array called traj with N elements
// It will return aTRAJECTORY_DATA structure with the maximum z value in yards, 
// the x value at z=0.0 (the impact x in yards), and the impact in degees.  The impact 
// angle is the is the angle at impact on the INSIDE of the curve

TRAJECTORY_DATA getTrajectoryData(POINT_XZ *traj, int N)
{
  TRAJECTORY_DATA impactData={0.0,0.0,0.0};

   return impactData;
}

