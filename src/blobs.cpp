#include <stdio.h>
#include "cv.h"
#include "highgui.h"
#include "blobs.h"



//***********************************************************//
//* Blob analysis package  Version1.3 5 January 2008        *//
//* Added:                                                  *//
//* - BLOBCOLOR                                             *//
//* History:                                                *//
//* - Version 1.0 8 August 2003                             *//
//* - Version 1.2 3 January 2008                            *//
//* - Version 1.3 5 January 2008                            *//
//* - Version 1.4 13 January 2008                           *//
//*                                                         *//
//* Input: IplImage* binary image                           *//
//* Output: attributes of each connected region             *//
//* Author: Dave Grossman                                   *//
//* Email: dgrossman@cdr.stanford.edu                       *//
//* Acknowledgement: the algorithm has been around > 25 yrs *//
//***********************************************************//

// Transfer fields from subsumed region to correct one


void Cblobs::Subsume(float RegionData[BLOBTOTALCOUNT][BLOBDATACOUNT],
			int HighRegionNum,
			int SubsumedRegion[BLOBTOTALCOUNT],
			int HiNum,
			int LoNum)
{
	if(HiNum > BLOBTOTALCOUNT) return;

	int iTargetTest;
	int iTargetValid = LoNum;

	while(TRUE)	// Follow subsumption chain to lowest number source
	{
		iTargetTest = SubsumedRegion[iTargetValid];
		if(iTargetTest < 0) break;
		iTargetValid = iTargetTest;
	}
	LoNum = iTargetValid;

	int i;
	for(i = BLOBAREA; i < BLOBDATACOUNT; i++)	// Skip over BLOBCOLOR
	{
		if(i == BLOBMINX || i == BLOBMINY)
		{
			if(RegionData[LoNum][i] > RegionData[HiNum][i]) { RegionData[LoNum][i] = RegionData[HiNum][i]; }
		}
		else if(i == BLOBMAXX || i == BLOBMAXY)
		{
		 	if(RegionData[LoNum][i] < RegionData[HiNum][i]) { RegionData[LoNum][i] = RegionData[HiNum][i]; }
		}
		else // Area, Perimeter, SumX, SumY, SumXX, SumYY, SumXY
		{
			RegionData[LoNum][i] += RegionData[HiNum][i];
		}
	}
	
	SubsumedRegion[HiNum] = LoNum;	// Mark dead region number for future compression
}

void Cblobs::PrintRegionDataArray(int option)
{

int ThisRegion;

if(option!=0)
{
	printf("\n      Parent-Color-Area-Perimeter-  X   -  Y   -Circularity\n");
	for(ThisRegion = 1; ThisRegion <= BlobCount; ThisRegion++)
	{
		printf("R=%3d: ", ThisRegion);
		printf("%3d  ", (int) RegionData[ThisRegion][BLOBPARENT]);		//parent
		printf("%3d  ", (int) RegionData[ThisRegion][BLOBCOLOR]);		//color
		printf("%6.1f  ",  RegionData[ThisRegion][BLOBAREA]);		//area
		printf("%6.1f  ",  RegionData[ThisRegion][BLOBPERIMETER]);		//perimeter
		printf("%6.1f  ",  RegionData[ThisRegion][BLOBSUMX]);		//X coordinate of center of mass
		printf("%6.1f  ",  RegionData[ThisRegion][BLOBSUMY]);		//y coordinate of center of mass
		printf("%6.2f  ",  RegionData[ThisRegion][BLOBCIRCULARITY]);		//circularity
		

		printf("\n");
	}
	printf("\n");

}
else

{

	printf("\nRegionDataArray\nParent-Color--Area---Perimeter---X-----Y-----BoundingBox\n");
	for(ThisRegion = 0; ThisRegion < BLOBTOTALCOUNT; ThisRegion++)
	{
		if(ThisRegion > 0 && RegionData[ThisRegion][0] < 0) break;
		if(RegionData[ThisRegion][BLOBAREA] <= 0) break;
		printf("R=%3d: ", ThisRegion);
		printf("%3d  ", (int) RegionData[ThisRegion][BLOBPARENT]);		//parent
		printf("%3d  ", (int) RegionData[ThisRegion][BLOBCOLOR]);		//color
		printf("%6d  ", (int) RegionData[ThisRegion][BLOBAREA]);		//area
		printf("%6d  ", (int) RegionData[ThisRegion][BLOBPERIMETER]);	//perimeter
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBSUMX]);	//sumx
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBSUMY]);	//sumy

		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMINX]);	//minx
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMAXX]);	//maxx
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMINY]);	//miny
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMAXY]);	//maxy
		printf("\n");
	}
	printf("\n");
}
}

int Cblobs::BlobAnalysis(IplImage* ImageHdr,	// input image
	
	int Col0, int Row0,					// start of ROI
	int Cols, int Rows,					// size of ROI (+2 for Tran array)
	uchar Border,						// border color
	int MinArea)						// max trans in any row
{

	// Display Gray image
	//cvNamedWindow("BlobInput", CV_WINDOW_AUTOSIZE);
	//cvShowImage("BlobInput", ImageHdr);
	

	if(Cols > BLOBCOLCOUNT) { return(-1); } // Bounds check - Error in column count
	if(Rows > BLOBROWCOUNT) { return(-2); }	// Bounds check - Error in row count
	
	char* Image = ImageHdr->imageData;
	int WidthStep = ImageHdr->widthStep; 

	// Convert image array into transition array. In each row
	// the transition array tells which columns have a color change
	int Trans = Cols;				// max trans in any row

	// row 0 and row Rows+1 represent the border
	int iTran, Tran;				// Data for a given run
	uchar ThisCell, LastCell;		// Contents (colors?) within this row
	int ImageOffset = WidthStep * Row0 - WidthStep - 1;	// Performance booster to avoid multiplication
	long int TransitionOffset = 0;		// Performance booster to avoid multiplication
	int iRow, iCol;
	int i;

	// Initialize Transition array
	for(i = 0; i < (Rows+2)*(Cols+2); i++) { WorkingStorage[i] = 0; };
	WorkingStorage[0] = WorkingStorage[(Rows+1)*(Cols+2)] = Cols + 2;

	// Fill Transition array
	for(iRow = Row0 + 1; iRow < Row0 + Rows + 1; iRow++)		// Choose a row of Bordered image
	{
		ImageOffset += WidthStep;	// Performance booster to avoid multiplication
		TransitionOffset += Cols + 2;// Performance booster to avoid multiplication
		iTran = 0;					// Index into Transition array
		Tran = 0;					// No transitions at row start

		if(TransitionOffset + Cols + 1 > WORKINGSTORAGE) break;	// Bounds check

		LastCell = Border;
		for(iCol = Col0; iCol < Col0 + Cols + 2; iCol++)	// Scan that row of Bordered image
		{
			if(iCol == Col0 || iCol == Col0 + Cols + 1) ThisCell = Border;
			else ThisCell = Image[ImageOffset + iCol];

			if(ThisCell != LastCell)
			{
				WorkingStorage[TransitionOffset + iTran] = Tran;	// Save completed Tran
				iTran++;						// Prepare new index
				LastCell = ThisCell;			// With this color
			}
			Tran++;	// Tran continues
		}
		WorkingStorage[TransitionOffset + iTran] = Tran;	// Save completed run
		WorkingStorage[TransitionOffset + iTran + 1] = -1;
	}
	// Process transition code depending on Last row and This row
	//
	// Last ---++++++--+++++++++++++++-----+++++++++++++++++++-----++++++-------+++---
	// This -----+++-----++++----+++++++++----+++++++---++------------------++++++++--
	//
	// There are various possibilities:
	//
	// Case     1       2       3       4       5       6       7       8
	// Last |xxx    |xxxxoo |xxxxxxx|xxxxxxx|ooxxxxx|ooxxx  |ooxxxxx|    xxx|
	// This |    yyy|    yyy|  yyyy |  yyyyy|yyyyyyy|yyyyyyy|yyyy   |yyyy   |
	// Here o is optional
	// 
	// Here are the primitive tests to distinguish these 6 cases:
	//   A) Last end < This start - 1 OR NOT		Note: -1
	//   B) This end < Last start OR NOT
	//   C) Last start < This start OR NOT
	//   D) This end < Last end OR NOT
	//   E) This end = Last end OR NOT
	//
	// Here is how to use these tests to determine the case:
	//   Case 1 = A [=> NOT B AND C AND NOT D AND NOT E]
	//   Case 2 = C AND NOT D AND NOT E [AND NOT A AND NOT B]
	//   Case 3 = C AND D [=> NOT E] [AND NOT A AND NOT B]
	//   Case 4 = C AND NOT D AND E [AND NOT A AND NOT B]
	//   Case 5 = NOT C AND E [=> NOT D] [AND NOT A AND NOT B]
	//   Case 6 = NOT C AND NOT D AND NOT E [AND NOT A AND NOT B]
	//   Case 7 = NOT C AND D [=> NOT E] [AND NOT A AND NOT B]
	//   Case 8 = B [=> NOT A AND NOT C AND D AND NOT E]
	//
	// In cases 2,3,4,5,6,7 the following additional test is needed:
	//   Match) This color = Last color OR NOT
	//
	// In cases 5,6,7 the following additional test is needed:
	//   Known) This region was already matched OR NOT
	//
	// Here are the main tests and actions:
	//   Case 1: LastIndex++;
	//   Case 2: if(Match) {y = x;}
	//           LastIndex++;
	//   Case 3: if(Match) {y = x;}
	//           else {y = new}
	//           ThisIndex++;
	//   Case 4: if(Match) {y = x;}
	//           else {y = new}
	//           LastIndex++;
	//           ThisIndex++;
	//   Case 5: if(Match AND NOT Known) {y = x}
	//           else if(Match AND Known) {Subsume(x,y)}
	//           LastIndex++;ThisIndex++
	//   Case 6: if(Match AND NOT Known) {y = x}
	//           else if(Match AND Known) {Subsume(x,y)}
	//           LastIndex++;
	//   Case 7: if(Match AND NOT Known) {y = x}
	//           else if(Match AND Known) {Subsume(x,y)}
	//           ThisIndex++;
	//   Case 8: ThisIndex++;

	// BLOBTOTALCOUNT is max num of regions incl all temps and background
	// BLOBROWCOUNT is the number of rows in the image
	// BLOBCOLCOUNT is the number of columns in the image
	// BLOBDATACOUNT is number of data elements for each region as follows:
	// BLOBPARENT 0	these are the respective indices for the data elements
	// BLOBCOLOR 1		0=background; 1=non-background
	// BLOBAREA 2
	// BLOBPERIMETER 3
	// BLOBSUMX 4		means
	// BLOBSUMY 5
	// BLOBSUMXX 6		2nd moments
	// BLOBSUMYY 7
	// BLOBSUMXY 8
	// BLOBMINX 9		bounding rectangle
	// BLOBMAXX 10
	// BLOBMINY 11
	// BLOBMAXY 12

	int SubsumedRegion[BLOBTOTALCOUNT];			// Blob result array
	int RenumberedRegion[BLOBTOTALCOUNT];		// Blob result array
	
	float ThisParent;	// These data can change when the line is current
	float ThisArea;
	float ThisPerimeter;
	float ThisSumX;
	float ThisSumY;
	float ThisSumXX;
	float ThisSumYY;
	float ThisSumXY;
	float ThisMinX;
	float ThisMaxX;
	float ThisMinY;
	float ThisMaxY;
	float LastPerimeter;	// This is the only data for retroactive change
	
	int HighRegionNum = 0;
	int RegionNum = 0;
	int ErrorFlag = 0;
	
	int LastRow, ThisRow;			// Row number
	int LastStart, ThisStart;		// Starting column of run
	int LastEnd, ThisEnd;			// Ending column of run
	int LastColor, ThisColor;		// Color of run
	
	int LastIndex, ThisIndex;		// Which run are we up to?
	int LastIndexCount, ThisIndexCount;	// Out of these runs
	int LastRegionNum, ThisRegionNum;	// Which assignment?
	int LastRegion[BLOBCOLCOUNT+2];	// Row assignment of region number
	int ThisRegion[BLOBCOLCOUNT+2];	// Row assignment of region number
	
	long int LastOffset = -(Trans + 2);	// For performance to avoid multiplication
	long int ThisOffset = 0;			// For performance to avoid multiplication
	int ComputeData;
	int j;

	for(i = 0; i < BLOBTOTALCOUNT; i++)	// Initialize result arrays
	{
		RenumberedRegion[i] = i;			// Initially no region is renumbered
		SubsumedRegion[i] = -1;				// Flag indicates region is not subsumed
		RegionData[i][0] = (float) -1;		// Flag indicates null region
		for(j = 1; j < BLOBDATACOUNT; j++)
		{
			if(j == BLOBMINX || j == BLOBMINY) RegionData[i][j] = (float) 1000000.0;
			else RegionData[i][j] = (float) 0.0;
		}
	}
	for(i = 0; i < BLOBROWCOUNT + 2; i++)	// Initialize result arrays
	{
		LastRegion[i] = -1;
		ThisRegion[i] = -1;
	}

	RegionData[0][BLOBPARENT] = (float) -1;
	RegionData[0][BLOBAREA] = (float) WorkingStorage[0];
	RegionData[0][BLOBPERIMETER] = (float) (2 + 2 * WorkingStorage[0]);

	ThisIndexCount = 1;
	ThisRegion[0] = 0;	// Border region

	// Initialize left border column
	for(i = Row0 + 1; i < Row0 + Rows + 2; i++) { ThisRegion[i] = -1; } // Flag as uninit
	
	// Loop over all rows
	for(ThisRow = Row0 + 1; ThisRow < Row0 + Rows + 2; ThisRow++)
	{
		ThisOffset += Trans + 2;
		ThisIndex = 0;
		
		LastOffset += Trans + 2;
		LastRow = ThisRow - 1;
		LastIndexCount = ThisIndexCount;
		LastIndex = 0;

		int EndLast = 0;
		int EndThis = 0;
		for(j = 0; j < Trans + 2; j++)
		{
			int Index = ThisOffset + j;
			int TranVal = WorkingStorage[Index];		// Run length
			if(TranVal > 0) ThisIndexCount = j + 1;	// stop at highest 

			if(ThisRegion[j] == -1)  { EndLast = 1; }
			if(TranVal < 0) { EndThis = 1; }

			if(EndLast > 0 && EndThis > 0) { break; }

			LastRegion[j] = ThisRegion[j];
			ThisRegion[j] = -1;		// Flag indicates region is not initialized
		}

		int MaxIndexCount = LastIndexCount;
		if(ThisIndexCount > MaxIndexCount) MaxIndexCount = ThisIndexCount;


		// Main loop over runs within Last and This rows
		while (LastIndex < LastIndexCount && ThisIndex < ThisIndexCount)
		{
			ComputeData = 0;
		
			if(LastIndex == 0) LastStart = 0;
			else LastStart = WorkingStorage[LastOffset + LastIndex - 1];
			LastEnd = WorkingStorage[LastOffset + LastIndex] - 1;
			LastColor = LastIndex - 2 * (LastIndex / 2);
			LastRegionNum = LastRegion[LastIndex];

			if(ThisIndex == 0) ThisStart = 0;
			else ThisStart = WorkingStorage[ThisOffset + ThisIndex - 1];
			ThisEnd = WorkingStorage[ThisOffset + ThisIndex] - 1;
			ThisColor = ThisIndex - 2 * (ThisIndex / 2);
			ThisRegionNum = ThisRegion[ThisIndex];

			if(ThisRegionNum >= BLOBTOTALCOUNT)	// Bounds check
			{
				ErrorFlag = -2; // Too many regions found - You must increase BLOBTOTALCOUNT
				break;
			}

			int TestA = (LastEnd < ThisStart - 1);	// initially false
			int TestB = (ThisEnd < LastStart);		// initially false
			int TestC = (LastStart < ThisStart);	// initially false
			int TestD = (ThisEnd < LastEnd);
			int TestE = (ThisEnd == LastEnd);

			int TestMatch = (ThisColor == LastColor);		// initially true
			int TestKnown = (ThisRegion[ThisIndex] >= 0);	// initially false

			int Case = 0;
			if(TestA) Case = 1;
			else if(TestB) Case = 8;
			else if(TestC)
			{
				if(TestD) Case = 3;
				else if(!TestE) Case = 2;
				else Case = 4;
			}
			else
			{
				if(TestE) Case = 5;
				else if(TestD) Case = 7;
				else Case = 6;
			}

			// Initialize common variables
			ThisArea = (float) 0.0;
			ThisSumX = ThisSumY = (float) 0.0;
			ThisSumXX = ThisSumYY = ThisSumXY = (float) 0.0;
			ThisMinX = ThisMinY = (float) 1000000.0;
			ThisMaxX = ThisMaxY = (float) -1.0;
			LastPerimeter = ThisPerimeter = (float) 0.0;
			ThisParent = (float) -1;

			// Determine necessary action and take it
			switch (Case)
			{ 
				case 1: //|xxx    |
						//|    yyy|
					
					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					LastIndex++;
					break;
					
					
				case 2: //|xxxxoo |
						//|    yyy|
					
					if(TestMatch)	// Same color
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						LastPerimeter = (float) LastEnd - ThisStart + 1;	// to subtract
						ThisPerimeter = 2 + 2 * ThisArea - LastPerimeter;
						ComputeData = 1;
					}
					
					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					LastIndex++;
					break;
					
					
				case 3: //|xxxxxxx|
						//|  yyyy |
					
					if(TestMatch)	// Same color
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						LastPerimeter = ThisArea;	// to subtract
						ThisPerimeter = 2 + ThisArea;
					}
					else		// Different color => New region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						ThisPerimeter = 2 + 2 * ThisArea;
					}
					
					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					ComputeData = 1;
					ThisIndex++;
					break;
					
					
				case 4:	//|xxxxxxx|
						//|  yyyyy|
					
					if(TestMatch)	// Same color
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						LastPerimeter = ThisArea;	// to subtract
						ThisPerimeter = 2 + ThisArea;
					}
					else		// Different color => New region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						ThisPerimeter = 2 + 2 * ThisArea;
					}
					
					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					ComputeData = 1;
					LastIndex++;
					ThisIndex++;
					break;
					
					
				case 5:	//|ooxxxxx|
						//|yyyyyyy|
					
					if(!TestMatch && !TestKnown)	// Different color and unknown => new region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						ThisPerimeter = 2 + 2 * ThisArea;
					}
					else if(TestMatch && !TestKnown)	// Same color and unknown
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						LastPerimeter = (float) LastEnd - LastStart + 1;	// to subtract
						ThisPerimeter = 2 + 2 * ThisArea - LastPerimeter;
						ComputeData = 1;
					}
					else if(TestMatch && TestKnown)	// Same color and known
					{
						LastPerimeter = (float) LastEnd - LastStart + 1;	// to subtract
						ThisPerimeter = - LastPerimeter;
						if(ThisRegionNum > LastRegionNum)
						{
							int iOld;
							Subsume(RegionData, HighRegionNum, SubsumedRegion, ThisRegionNum, LastRegionNum);
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == ThisRegionNum) ThisRegion[iOld] = LastRegionNum;
								if(LastRegion[iOld] == ThisRegionNum) LastRegion[iOld] = LastRegionNum;
							}					
							ThisRegionNum = LastRegionNum;
						}
						else if(ThisRegionNum < LastRegionNum)
						{
							int iOld;
							Subsume(RegionData, HighRegionNum, SubsumedRegion, LastRegionNum, ThisRegionNum);
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == LastRegionNum) ThisRegion[iOld] = ThisRegionNum;
								if(LastRegion[iOld] == LastRegionNum) LastRegion[iOld] = ThisRegionNum;
							}					
							LastRegionNum = ThisRegionNum;
						}
					}

					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					LastIndex++;
					ThisIndex++;
					break;
					
					
				case 6:	//|ooxxx  |
						//|yyyyyyy|

					if(TestMatch && !TestKnown)
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						LastPerimeter = (float) LastEnd - LastStart + 1;	// to subtract
						ThisPerimeter = 2 + 2 * ThisArea - LastPerimeter;
						ComputeData = 1;
					}
					else if(TestMatch && TestKnown)
					{
						LastPerimeter = (float) LastEnd - LastStart + 1;	// to subtract
						ThisPerimeter = - LastPerimeter;
						if(ThisRegionNum > LastRegionNum)
						{
							int iOld;
							Subsume(RegionData, HighRegionNum, SubsumedRegion, ThisRegionNum, LastRegionNum);
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == ThisRegionNum) ThisRegion[iOld] = LastRegionNum;
								if(LastRegion[iOld] == ThisRegionNum) LastRegion[iOld] = LastRegionNum;
							}					
							ThisRegionNum = LastRegionNum;
						}
						else if(ThisRegionNum < LastRegionNum)
						{
							Subsume(RegionData, HighRegionNum, SubsumedRegion, LastRegionNum, ThisRegionNum);
							int iOld;
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == LastRegionNum) ThisRegion[iOld] = ThisRegionNum;
								if(LastRegion[iOld] == LastRegionNum) LastRegion[iOld] = ThisRegionNum;
							}					
							LastRegionNum = ThisRegionNum;
						}
					}

					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					LastIndex++;
					break;
					
					
				case 7:	//|ooxxxxx|
						//|yyyy   |
					
					if(!TestMatch && !TestKnown)	// Different color and unknown => new region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						ThisPerimeter = 2 + 2 * ThisArea;
					}
					else if(TestMatch && !TestKnown)
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						ThisPerimeter = 2 + ThisArea;
						LastPerimeter = (float) ThisEnd - LastStart + 1;
						ThisPerimeter = 2 + 2 * ThisArea - LastPerimeter;
						ComputeData = 1;
					}
					else if(TestMatch && TestKnown)
					{
						LastPerimeter = (float) ThisEnd - LastStart + 1;	// to subtract
						ThisPerimeter = - LastPerimeter;
						if(ThisRegionNum > LastRegionNum)
						{
							Subsume(RegionData, HighRegionNum, SubsumedRegion, ThisRegionNum, LastRegionNum);
							int iOld;
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == ThisRegionNum) ThisRegion[iOld] = LastRegionNum;
								if(LastRegion[iOld] == ThisRegionNum) LastRegion[iOld] = LastRegionNum;
							}					
							ThisRegionNum = LastRegionNum;
						}
						else if(ThisRegionNum < LastRegionNum)
						{
							Subsume(RegionData, HighRegionNum, SubsumedRegion, LastRegionNum, ThisRegionNum);
							int iOld;
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == LastRegionNum) ThisRegion[iOld] = ThisRegionNum;
								if(LastRegion[iOld] == LastRegionNum) LastRegion[iOld] = ThisRegionNum;
							}					
							LastRegionNum = ThisRegionNum;
						}
					}

					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					ThisIndex++;
					break;
					
				case 8:	//|    xxx|
						//|yyyy   |
					
					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					ThisIndex++;
					break;
					
				default:
					ErrorFlag = -1;	// Impossible case 
					break;
			}	// end switch case
			if(ErrorFlag != 0) break;

			if(ComputeData > 0)
			{
				int k;
				for(k = ThisStart; k <= ThisEnd; k++)
				{
					ThisSumX += (float) (k - 1);
					ThisSumXX += (float) (k - 1) * (k - 1);
				}
				float ImageRow = (float) (ThisRow - 1);

				ThisSumXY = ThisSumX * ImageRow;
				ThisSumY = ThisArea * ImageRow;
				ThisSumYY = ThisSumY * ImageRow;
					
				if(ThisStart - 1 < (int) ThisMinX) ThisMinX = (float) (ThisStart - 1);
				if(ThisMinX < (float) 0.0) ThisMinX = (float) 0.0;
				if(ThisEnd - 1 > (int) ThisMaxX) ThisMaxX = (float) (ThisEnd - 1);

				if(ImageRow < ThisMinY) ThisMinY = ImageRow;
				if(ThisMinY < (float) 0.0) ThisMinY = (float) 0.0;
				if(ImageRow > ThisMaxY) ThisMaxY = ImageRow;
			}

			if(ThisRegionNum >= 0)
			{

				if(ThisRegionNum >= BLOBTOTALCOUNT) // Too many regions found - You must increase BLOBTOTALCOUNT
				{
					ErrorFlag = -2;
					break;
				}


				if(ThisParent >= 0) { RegionData[ThisRegionNum][BLOBPARENT] = (float) ThisParent; }
				RegionData[ThisRegionNum][BLOBCOLOR] = (float) ThisColor;	// New code
				RegionData[ThisRegionNum][BLOBAREA] += ThisArea;
				RegionData[ThisRegionNum][BLOBPERIMETER] += ThisPerimeter;
				
				if(ComputeData > 0)
				{
					RegionData[ThisRegionNum][BLOBSUMX] += ThisSumX;
					RegionData[ThisRegionNum][BLOBSUMY] += ThisSumY;
					RegionData[ThisRegionNum][BLOBSUMXX] += ThisSumXX;
					RegionData[ThisRegionNum][BLOBSUMYY] += ThisSumYY;
					RegionData[ThisRegionNum][BLOBSUMXY] += ThisSumXY;
					RegionData[ThisRegionNum][BLOBPERIMETER] -= LastPerimeter;
					if(RegionData[ThisRegionNum][BLOBMINX] > ThisMinX) RegionData[ThisRegionNum][BLOBMINX] = ThisMinX;
					if(RegionData[ThisRegionNum][BLOBMAXX] < ThisMaxX) RegionData[ThisRegionNum][BLOBMAXX] = ThisMaxX;
					if(RegionData[ThisRegionNum][BLOBMINY] > ThisMinY) RegionData[ThisRegionNum][BLOBMINY] = ThisMinY;
					if(RegionData[ThisRegionNum][BLOBMAXY] < ThisMaxY) RegionData[ThisRegionNum][BLOBMAXY] = ThisMaxY;
				}
			}

		}	// end Main loop
		if(ErrorFlag != 0) break;

	}	// end Loop over all rows

	// Subsume regions that have too small area
	int HiNum;
	for(HiNum = HighRegionNum; HiNum > 0; HiNum--)
	{
		
		if(SubsumedRegion[HiNum] < 0 && RegionData[HiNum][BLOBAREA] < (float) MinArea)
		{
			Subsume(RegionData, HighRegionNum, SubsumedRegion, HiNum, (int) RegionData[HiNum][BLOBPARENT]);
		}
	}

	// Compress region numbers to eliminate subsumed regions
	int iOld;
	int iNew = 0;
	for(iOld = 0; iOld <= HighRegionNum; iOld++)
	{
		if(SubsumedRegion[iOld] >= 0) {	continue; }	// Region subsumed, empty, no further action
		else
		{
			int iTargetTest;
			int iTargetValid = (int)  RegionData[iOld][BLOBPARENT];
			while(TRUE)	// Follow subsumption chain
			{
				iTargetTest = SubsumedRegion[iTargetValid];
				if(iTargetTest < 0) break;
				iTargetValid = iTargetTest;
			}
			RegionData[iOld][BLOBPARENT] = (float) RenumberedRegion[iTargetValid];

			// Move data from old region number to new region number
			int j;
			for(j = 0; j < BLOBDATACOUNT; j++) { RegionData[iNew][j] = RegionData[iOld][j]; }
			RenumberedRegion[iOld] = iNew;
			iNew++;
		}
	}
	HighRegionNum = iNew - 1;				// Update where the data ends
	RegionData[HighRegionNum + 1][0] = -1;	// and set end of array flag

	// Normalize summation fields into moments 
	for(ThisRegionNum = 0; ThisRegionNum <= HighRegionNum; ThisRegionNum++)
	{
		// Extract fields
		float Area = RegionData[ThisRegionNum][BLOBAREA];
		float SumX = RegionData[ThisRegionNum][BLOBSUMX];
		float SumY = RegionData[ThisRegionNum][BLOBSUMY];
		float SumXX = RegionData[ThisRegionNum][BLOBSUMXX];
		float SumYY = RegionData[ThisRegionNum][BLOBSUMYY];
		float SumXY = RegionData[ThisRegionNum][BLOBSUMXY];
	
		// Get averages
		SumX /= Area;
		SumY /= Area;
		SumXX /= Area;
		SumYY /= Area;
		SumXY /= Area;

		// Create moments
		SumXX -= SumX * SumX;
		SumYY -= SumY * SumY;
		SumXY -= SumX * SumY;
		if(SumXY > -1.0E-14 && SumXY < 1.0E-14)
		{
			SumXY = (float) 0.0; // Eliminate roundoff error
		}
		RegionData[ThisRegionNum][BLOBSUMX] = SumX;
		RegionData[ThisRegionNum][BLOBSUMY] = SumY;
		RegionData[ThisRegionNum][BLOBSUMXX] = SumXX;
		RegionData[ThisRegionNum][BLOBSUMYY] = SumYY;
		RegionData[ThisRegionNum][BLOBSUMXY] = SumXY;

		RegionData[ThisRegionNum][BLOBSUMX] += Col0;
		RegionData[ThisRegionNum][BLOBMINX] += Col0;
		RegionData[ThisRegionNum][BLOBMAXX] += Col0;

		float h = (SumXX + SumYY) * .5;
		float h2= sqrt ( h*h - SumXX * SumYY + SumXY * SumXY);
		RegionData[ThisRegionNum][BLOBMAJORAXIS] = sqrt(h + h2);
		RegionData[ThisRegionNum][BLOBMINORAXIS] = sqrt(h - h2);
		

		float e=sqrt((SumXX - SumYY) * (SumXX - SumYY) + 4 * SumXY*SumXY);
		
		RegionData[ThisRegionNum][BLOBECCENTRICITY]= (SumXX+SumYY+e)/(SumXX+SumYY-e);

		float rect_area=(RegionData[ThisRegionNum][BLOBMAXX]-RegionData[ThisRegionNum][BLOBMINX])*(RegionData[ThisRegionNum][BLOBMAXY]-RegionData[ThisRegionNum][BLOBMINY]);
		RegionData[ThisRegionNum][BLOBCOMPACTNESS] =Area/rect_area;

		float angle=(atan(2*SumXY/(SumXX-SumYY))/2)*180/CV_PI;
		if( RegionData[ThisRegionNum][BLOBMAXX]-RegionData[ThisRegionNum][BLOBMINX]>RegionData[ThisRegionNum][BLOBMAXY]-RegionData[ThisRegionNum][BLOBMINY])
			angle=180-angle;
		else
			angle=90-angle;
		RegionData[ThisRegionNum][BLOBORIENTATION] = angle;
		
	}

	for(ThisRegionNum = HighRegionNum; ThisRegionNum > 0 ; ThisRegionNum--)
	{
		// Subtract interior perimeters
		int ParentRegionNum = (int) RegionData[ThisRegionNum][BLOBPARENT];
		RegionData[ParentRegionNum][BLOBPERIMETER] -= RegionData[ThisRegionNum][BLOBPERIMETER];
		RegionData[ThisRegionNum][BLOBCIRCULARITY] =20.5*RegionData[ThisRegionNum][BLOBAREA]/(RegionData[ThisRegionNum][BLOBPERIMETER]*RegionData[ThisRegionNum][BLOBPERIMETER]);

	}

	RegionData[HighRegionNum+1][BLOBPARENT] = -2;

	if(ErrorFlag != 0) return(ErrorFlag);

	BlobCount=HighRegionNum;

	return(HighRegionNum);
 }




//Witek
int Cblobs::BlobInclude(int Criterion, double Low_threshold, double High_threshold)
{
	BlobCountTmp=1;

	if(Criterion==BLOBPARENT)
	{
		for(int i=1;i<=BlobCount;i++)
		{
			if(RegionData[i][BLOBPARENT]>=Low_threshold && RegionData[i][BLOBPARENT]<=High_threshold)
			{
				for(int j=0;j<BLOBDATACOUNT;j++)
					RegionDataTmp[BlobCountTmp][j]=RegionData[i][j];
				BlobCountTmp++;
			}

		}
		
	}

		//copying filtered blobs to the original array
	BlobCountTmp-=1;
	BlobCount=0;
	for(int i=1;i<=BlobCountTmp;i++)
	{
		BlobCount++;
		for(int j=0;j<BLOBDATACOUNT;j++)
			RegionData[BlobCount][j]=RegionDataTmp[i][j];
		
	}
	//after last correct record putting one that has a negative parent
	RegionData[BlobCount+1][0]=-2;

		
	return(1);

}

int Cblobs::BlobExclude(int Criterion, double Low_threshold, double High_threshold)
{
	//filters found blobs based on given criteria
	//for a single parameter criterion only Low_threshold is taken into consideration
	//fultering is done in a very simple but not optimal way - by creating another array of filtered
	//blobs and copying it back ont the original - NEEDS OPTIMIZATION
	BlobCountTmp=1;

	switch(Criterion)
	{
	
	case BLOBAREA:
	case BLOBPERIMETER:
	case BLOBCOMPACTNESS:
	case BLOBPARENT:
	case BLOBECCENTRICITY:
	case BLOBCIRCULARITY:
		for(int i=1;i<=BlobCount;i++)
			if(RegionData[i][Criterion]<Low_threshold || RegionData[i][Criterion]>High_threshold)
			{
				for(int j=0;j<BLOBDATACOUNT;j++)
					RegionDataTmp[BlobCountTmp][j]=RegionData[i][j];
				BlobCountTmp++;
			}
			break;

	case BLOBCOLOR:
		for(int i=1;i<=BlobCount;i++)
			if(RegionData[i][BLOBCOLOR]!=Low_threshold)
			{
				for(int j=0;j<BLOBDATACOUNT;j++)
					RegionDataTmp[BlobCountTmp][j]=RegionData[i][j];
				BlobCountTmp++;
			}
			break;

	case BLOBSIBLING:
		//najpierw wyznaczamy rodzenstwo odfiltrowanych blobow, a nie wszystkich, tylko aktualnych
		//na poczatku kazdy jest jedynakiem
		for(int i=0;i<BlobCount;i++)
			RegionData[i][BLOBSIBLING]=1;
		for(int i=1;i<=BlobCount;i++)
			for(int j=1;j<=BlobCount;j++)
			{
				if(i==j) continue;
				if(RegionData[i][BLOBPARENT]==RegionData[j][BLOBPARENT])
					RegionData[i][BLOBSIBLING]++;
			}
			//teraz nastepuje odfiltrowanie
			for(int i=1;i<=BlobCount;i++)
				if(RegionData[i][BLOBSIBLING]<Low_threshold || RegionData[i][BLOBSIBLING]>High_threshold)
				{
					for(int j=0;j<BLOBDATACOUNT;j++)
						RegionDataTmp[BlobCountTmp][j]=RegionData[i][j];
					BlobCountTmp++;
				}

				break;
	
	
	}

	//copying filtered blobs to the original array
	BlobCountTmp-=1;
	BlobCount=0;
	for(int i=1;i<=BlobCountTmp;i++)
	{
		BlobCount++;
		for(int j=0;j<BLOBDATACOUNT;j++)
			RegionData[BlobCount][j]=RegionDataTmp[i][j];
		
	}
	//after last correct record putting one that has a negative parent
	RegionData[BlobCount+1][0]=-2;

	return(1);
}
