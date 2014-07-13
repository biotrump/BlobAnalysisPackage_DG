//***********************************************************//
//* Blob analysis package  Version1.41 13 January 2008        *//
//* Added:                                                  *//
//* - BLOBCOLOR                                             *//
//* History:                                                *//
//* - Version 1.0 8 August 2003                             *//
//* - Version 1.2 3 January 2008                            *//
//* - Version 1.3 5 January 2008                            *//
//* - Version 1.4 13 January 2008							*//
//* - Version 1.41 16 April 2009                            *//
//*                                                         *//
//* Input: IplImage* binary image                           *//
//* Output: attributes of each connected region             *//
//* Author: Dave Grossman                                   *//
//* Email: dgrossman@cdr.stanford.edu                       *//
//* Acknowledgement: the algorithm has been around > 25 yrs *//
//* - Version 1.41 is modified from original 1.4            *//
//* by Witold Czajewski										*//
//* Email: W.Czajewski@isep.pw.edu.pl						*//
//***********************************************************//

// defines for blob array sizes and indices


#ifdef ROWS
#define BLOBROWCOUNT ROWS
#endif

#ifdef COLS
#define BLOBCOLCOUNT COLS
#endif

#ifndef BLOBROWCOUNT
#define BLOBROWCOUNT 3040
#endif

#ifndef BLOBCOLCOUNT
#define BLOBCOLCOUNT 3040
#endif

#ifndef BLOBTOTALCOUNT
#define BLOBTOTALCOUNT (BLOBROWCOUNT + BLOBCOLCOUNT) * 5
#endif

#define WORKINGSTORAGE (BLOBROWCOUNT+2)*(BLOBCOLCOUNT+2)

#define BLOBPARENT 0
#define BLOBCOLOR 1
#define BLOBAREA 2
#define BLOBPERIMETER 3
#define BLOBSUMX 4
#define BLOBSUMY 5
#define BLOBSUMXX 6
#define BLOBSUMYY 7
#define BLOBSUMXY 8
#define BLOBMINX 9
#define BLOBMAXX 10
#define BLOBMINY 11
#define BLOBMAXY 12
#define BLOBMAJORAXIS 13
#define BLOBMINORAXIS 14
#define BLOBORIENTATION 15
#define BLOBECCENTRICITY 16
#define BLOBCOMPACTNESS 17
#define BLOBCIRCULARITY 18
#define BLOBSIBLING 19
#define BLOBDATACOUNT 20

class Cblobs
{

public:

// Global variables to avoid memory leak
int WorkingStorage[WORKINGSTORAGE];					// Working storage Note +2 +2 for image border
float RegionData[BLOBTOTALCOUNT][BLOBDATACOUNT];	// Blob result array

float RegionDataTmp[BLOBTOTALCOUNT][BLOBDATACOUNT]; //Witek - temporary array for blob filtering
int BlobCount, BlobCountTmp;						//Witek - global counters to make things easier

// Subroutine prototypes
void PrintRegionDataArray(int option=0);
void Subsume(float[BLOBTOTALCOUNT][BLOBDATACOUNT], int, int[BLOBTOTALCOUNT], int, int);
int BlobAnalysis(IplImage*, int, int, int, int, uchar, int);

//Witek - blob filtering functions
int BlobInclude(int Criterion, double Low_threshold, double High_threshold=0);
int BlobExclude(int Criterion, double Low_threshold, double High_threshold=0);
};