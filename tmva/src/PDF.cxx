/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : TMVA::PDF                                                             *
 *                                                                                *
 * Description:                                                                   *
 *      Implementation (see header for description)                               *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Andreas Hoecker <Andreas.Hocker@cern.ch> - CERN, Switzerland              *
 *      Xavier Prudent  <prudent@lapp.in2p3.fr>  - LAPP, France                   *
 *      Helge Voss      <Helge.Voss@cern.ch>     - MPI-KP Heidelberg, Germany     *
 *      Kai Voss        <Kai.Voss@cern.ch>       - U. of Victoria, Canada         *
 *                                                                                *
 * Copyright (c) 2005:                                                            *
 *      CERN, Switzerland,                                                        * 
 *      U. of Victoria, Canada,                                                   * 
 *      MPI-KP Heidelberg, Germany,                                               * 
 *      LAPP, Annecy, France                                                      *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://mva.sourceforge.net/license.txt)                                       *
 *                                                                                *
 **********************************************************************************/

#include <iostream> // Stream declarations

#include "TMVA/PDF.h"
#include "TMVA/TSpline1.h"
#include "TMVA/TSpline2.h"
#include "TH1F.h"
#include "TH1D.h"

namespace TMVA {
   const Bool_t   DEBUG_PDF=kFALSE;
   const Double_t PDF_epsilon_=1.0e-02;
   const Int_t    NBIN_PdfHist_=10000;
}

using namespace std;

ClassImp(TMVA::PDF)

//_______________________________________________________________________
   TMVA::PDF::PDF( const TH1 *hist, TMVA::PDF::SmoothMethod method, Int_t nsmooth )
      : fNsmooth ( nsmooth ),
        fSpline  ( 0 ),
        fPDFHist ( 0 ),
        fHist    ( 0 ),
        fGraph   ( 0 ),
        fIntegral( 1.0)
{  
   // constructor: 
   // - default Spline method is: Spline2 (quadratic)
   // - default smoothing is none

   // sanity check
   if (hist == NULL) {
      cout << "--- TMVA::PDF: ERROR!!! Called without valid histogram pointer!" << endl;
      exit(1);
   }
   fNbinsPDFHist = NBIN_PdfHist_;
   fHist = (TH1*)hist->Clone();
  
   // check histogram!
   CheckHist();
    
   // use ROOT TH1 smooth methos
   if (fNsmooth >0) fHist->Smooth( fNsmooth );
  
   // fill histogramm to graph
   fGraph = new TGraph( hist );
    
   switch (method) {

   case TMVA::PDF::kSpline1:
      fSpline = new TMVA::TSpline1( "spline1", fGraph );
      break;

   case TMVA::PDF::kSpline2:
      fSpline = new TMVA::TSpline2( "spline2", fGraph );
      break;

   case TMVA::PDF::kSpline3:
      fSpline = new TSpline3    ( "spline3", fGraph );
      break;
    
   case TMVA::PDF::kSpline5:
      fSpline = new TSpline5    ( "spline5", fGraph );
      break;

   default:
      cout << "--- " << GetName() 
           << ": Warning no valid interpolation method given! Use Spline3" << endl;
      fSpline = new TSpline3    ( "spline3", fGraph );
   }

   // fill into histogram 
   FillSplineToHist();

   fSpline->SetTitle( (TString)hist->GetTitle() + fSpline->GetTitle() );
   fSpline->SetName ( (TString)hist->GetName()  + fSpline->GetName()  );

   // normalize
   Integral();
   fPDFHist->Scale( 1.0/fIntegral );
}

//_______________________________________________________________________
TMVA::PDF::~PDF( void )
{
   // destructor
   if (fSpline  != NULL) delete fSpline ;
   if (fHist    != NULL) delete fHist;
   //  if (fGraph   != NULL) delete fGraph;
   if (fPDFHist != NULL) delete fPDFHist;
}

//_______________________________________________________________________
void TMVA::PDF::FillSplineToHist( void )
{
   // creates high-binned reference histogram to be used in stead of the 
   // PDF for speed reasons 

   fPDFHist = new TH1D( "", "", fNbinsPDFHist, fXmin, fXmax );
   fPDFHist->SetTitle( (TString)fHist->GetTitle() + "_hist from_" + fSpline->GetTitle() );
   fPDFHist->SetName ( (TString)fHist->GetName()  + "_hist_from_" + fSpline->GetTitle() );

   for (Int_t bin=1; bin <= fNbinsPDFHist; bin++) {
      Double_t x = fPDFHist->GetBinCenter( bin );
      Double_t y = fSpline->Eval( x );
      // sanity correction: in cases where strong slopes exist, accidentally, the 
      // splines can go to zero; in this case we set the corresponding bin content
      // equal to the bin content of the original histogram
      if (y <= TMVA::PDF_epsilon_) y = fHist->GetBinContent( fHist->FindBin( x ) );
      fPDFHist->SetBinContent( bin, TMath::Max(y, TMVA::PDF_epsilon_) );
   }
}

//_______________________________________________________________________
void TMVA::PDF::CheckHist(void)
{
   // sanity check: compare PDF with original histogram
   if (fHist == NULL) {
      cout << "--- " << GetName() 
           << ": CheckHist: ERROR!!! Called without valid histogram pointer!" << endl;
      exit(1);
   }

   // store basic quanities of input histogram
   fXmin = fHist->GetXaxis()->GetXmin();
   fXmax = fHist->GetXaxis()->GetXmax();
   Int_t nbins = fHist->GetNbinsX();

   Int_t emptyBins=0;
   // count number of empty bins
   for(Int_t bin=1; bin<=nbins; bin++) 
      if (fHist->GetBinContent(bin) == 0) emptyBins++;

   if (((Float_t)emptyBins/(Float_t)nbins) > 0.5) {
      cout << "--- " << GetName() 
           << ": WARNING More than 50% ("<<(((Float_t)emptyBins/(Float_t)nbins)*100)
           <<"%) of the bins in hist '" 
           << fHist->GetName() << "' are empty!" << endl;
      cout << "--- " << GetName() 
           << ": X_min=" << fXmin 
           << " mean=" << fHist->GetMean() << " X_max= " << fXmax << endl;  
   }

   if (DEBUG_PDF) 
      cout << "--- " << GetName() << ": "    
           << fXmin << " < x < " << fXmax << " in " << nbins << " bins" << endl;
}

//_______________________________________________________________________
Double_t  TMVA::PDF::Integral( void )
{
   // computes normalisation
   return GetIntegral( fXmin, fXmax );
}

//_______________________________________________________________________
Double_t TMVA::PDF::GetIntegral( Double_t xmin, Double_t xmax ) 
{  
   // computes PDF integral within given ranges
   Double_t  integral = 0;
   Int_t     nsteps   = 10000;
   Double_t  intBin   = (xmax - xmin)/nsteps; // bin width for integration
   for (Int_t bini=0; bini < nsteps; bini++) {
      Double_t x = (bini + 0.5)*intBin + xmin;
      integral += GetVal( x );
   }
   integral *= intBin;
  
   return integral;
}

//_______________________________________________________________________
Double_t TMVA::PDF::GetVal( const Double_t x )
{  
   // returns value PDF(x)

   // check which is filled
   Int_t bin       = fPDFHist->FindBin(x);
   if      (bin < 1             ) bin = 1;
   else if (bin > fNbinsPDFHist) bin = fNbinsPDFHist;

   Int_t nextbin   = bin;
   if ((x > fPDFHist->GetBinCenter(bin) && bin != fNbinsPDFHist) || bin == 1) 
      nextbin++;
   else
      nextbin--;  

   //sanity check
   if (fIntegral <= 0.0) fIntegral = 1.0;

   // linear interpolation between adjacent bins
   Double_t dx     = fPDFHist->GetBinCenter(bin)  - fPDFHist->GetBinCenter(nextbin);
   Double_t dy     = fPDFHist->GetBinContent(bin) - fPDFHist->GetBinContent(nextbin);
   Double_t retval = fPDFHist->GetBinContent(bin) + (x - fPDFHist->GetBinCenter(bin))*dy/dx;

   return max(retval, TMVA::PDF_epsilon_);
}




