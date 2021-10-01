#include <iostream>
#include <err.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <TString.h>
#include <TApplication.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TMarker.h>
using namespace std;

#include "inc/shinclude.h"

#define HEADER_SIZE (4*4)
#define DATA_SIZE (1024*1024)
#define N_XCHANNEL 16
#define N_YCHANNEL 16
#define N_CHANNEL 32

#define N_SAMPLE 60
//#define BASELINE1
#define BASELINE2
#define BASELINE1_INI 0
#define BASELINE1_END 9
#define BASELINE2_INI 0
#define BASELINE2_END 9
#define SIGNAL_INI 9
#define SIGNAL_END 59

#define mean 0.5
#define mean1 16.5

#define NCHAN_THR 3
#define NCHAN_THR1 9
#define NCHAN_THR_3X 3
#define NCHAN_THR_3X1 8

#define TARGET_EVT 100

/*
  #define ARG1 0.405
  #define ARG2 0.355
  #define ARG3 0.165
  #define ARG4 0.47
*/

#define PI 3.141592
#define ARG1 0.405
#define ARG2 0.355
#define ARG3 0.165
#define ARG4 0.47
//#define TOTAL 553000

#define DRAW_MODE 2

/* MODULE Layout
   +---+|||+
   | 3 | 1 |
   +---+---+
   | 4 | 2 |
   +---+---+
   
   Dead channel 
   MODULE1(m.dat):X-9ch(22ch)
   MODULE2(s.dat):X-1ch(30ch)
   MODULE3(s2.dat):X-2ch...8ch, X-12ch(21ch)
   MODULE4(s3.dat):Y-3ch(3ch)

   MODULE0:Don't adapt
*/


#define DEADCHAN_1 8
#define DEADCHAN_2 0
#define DEADCHAN_3 2
#define DEADCHAN_4 11
#define DEADCHAN_5 3
//#define BASELINE


/* Data Format
 *         32bit
 * +-------------------+
 * |       Unused      |  buf[0] - buf[3] : unused
 * +-------------------+
 * |    |    |    |    |  buf[4]: 0x01, buf[5] - buf[7]: packet id
 * +-------------------+
 * |     Data Length   |  Data Length (only the data part in bytes size)
 * +-------------------+
 * |   trigger Counter |
 * +-------------------+
 * |  CH 0   |  CH 1   |
 * +-------------------+
 * |         |         |
 * +-------------------+
 * |         |         |
 * +-------------------+
 * |         |  CH 32  |
 * +-------------------+
 * |  CH 0   |  CH 1   |
 * +-------------------+
 * |         |         |
 * +-------------------+
 * |         |         |
 * +-------------------+
 * |         |  CH 32  |
 * +-------------------+
 *   Data part follows
 */

int usage()
{
    cerr << "Usage: prog filename" << endl;
    cerr << "Print module_number channel_number trigger_counter data" << endl;
    return 0;
}


double xCoordinate( int module, double xChan, double yChan ){
    double abs,xcoor,ycoor,xrot,arg1,arg2,arg3,arg4;
    if (module == 1)
        {
            /*
              arg1 = ARG1 * PI/180.0;
              xcoor = 0.55 * xChan + 0.1;
              ycoor = -0.55 * yChan - 0.1;
              // abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
              //xrot = (xcoor-0.1)*(cos(arg1))+(ycoor+0.1)*(sin(arg1))+0.1;
              return xcoor;//xrot+0.321;//0.321
            */
      
            arg1 = ARG1 * PI/180.0;
            xcoor = 0.55 * xChan + 0.321;
            ycoor = -0.55 * yChan - 0.138;
            xrot = (xcoor-0.321)*(cos(arg1))+(ycoor+0.138)*(sin(arg1))+0.321;
            return xrot;
      
        }
    else if (module == 2)
        {
      
            arg2 = ARG2 * PI/180.0;
            xcoor =  -0.55 * yChan;// - 0.1;
            ycoor = -0.55 * xChan;// - 0.1;
            xrot = (xcoor)*(cos(arg2))-(ycoor)*(sin(arg2));
            return xrot;
      
            /*
              arg2 = ARG2 * PI/180.0;
              xcoor =  -0.55 * yChan;// - 0.1;
              ycoor = -0.55 * xChan;// - 0.1;
              //abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
              xrot = (xcoor+0.1)*(cos(arg2))-(ycoor+0.1)*(sin(arg2))-0.1;
              return xrot;
            */
        }
    else if (module == 3)
        {
            /* 
               arg3 = ARG3 * PI/180.0;
               xcoor =  0.55 * yChan + 0.1;
               ycoor = 0.55 * xChan + 0.1;
               // abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
               //xrot = (xcoor-0.1)*(cos(arg3))-(ycoor-0.1)*(sin(arg3))+0.1;     
               return xcoor;//xrot+0.252;//0.252
            */
       
            arg3 = ARG3 * PI/180.0;
            xcoor =  0.55 * yChan + 0.252;
            ycoor = 0.55 * xChan + 0.206;
            xrot = (xcoor-0.252)*(cos(arg3))-(ycoor-0.206)*(sin(arg3))+0.252;     
            return xrot;
       
        }
    else if (module == 4)
        {
            /*
              arg4 = ARG4 * PI/180.0;
              xcoor =  -0.55 * xChan - 0.1;
              ycoor = 0.55 * yChan + 0.1;
              //abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
              //xrot = (xcoor+0.1)*(cos(arg4))-(ycoor-0.1)*(sin(arg4))-0.1;
              return xcoor;//xrot+0.069;
            */
            arg4 = ARG4 * PI/180.0;
            xcoor =  -0.55 * xChan + 0.069;
            ycoor = 0.55 * yChan + 0.229;
            xrot = (xcoor-0.069)*(cos(arg4))-(ycoor-0.229)*(sin(arg4))+0.069;
            return xrot;
       
        }
    else {
        cerr << "ERROR: Wrong module number = " << module << endl;
        return -999.;
    }
}

double yCoordinate( int module, double xChan, double yChan ){
    double abs,xcoor,ycoor,yrot,arg1,arg2,arg3,arg4;
    if (module == 1)
        {
            /*
              arg1 = ARG1 * PI/180.0;
              xcoor = 0.55 * xChan + 0.1;
              ycoor = -0.55 * yChan - 0.1;
              //abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
              //yrot = -(xcoor-0.1)*(sin(arg1))+(ycoor+0.1)*(cos(arg1))-0.1;
              return ycoor;//yrot-0.138;
            */
      
            arg1 = ARG1 * PI/180.0;
            xcoor = 0.55 * xChan + 0.321;
            ycoor = -0.55 * yChan - 0.138;
            yrot = -(xcoor-0.321)*(sin(arg1))+(ycoor+0.138)*(cos(arg1))-0.138;
            return  yrot;
      
        }
    else if (module == 2)
        {
            /*
              arg2 = ARG2 * PI/180.0;
              xcoor =  -0.55 * yChan - 0.1;
              ycoor = -0.55 * xChan - 0.1;
              // abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
              //yrot = (xcoor+0.1)*(sin(arg2))+(ycoor+0.1)*(cos(arg2))-0.1;
              return ycoor;//yrot;
            */
     
            arg2 = ARG2 * PI/180.0;
            xcoor =  -0.55 * yChan;
            ycoor = -0.55 * xChan;
            yrot = (xcoor)*(sin(arg2))+(ycoor)*(cos(arg2));
            return yrot;
     
        }
    else if (module == 3)
        {
            /*
              arg3 = ARG3 * PI/180.0;
              xcoor =  0.55 * yChan + 0.1;
              ycoor = 0.55 * xChan + 0.1;
              // abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
              //yrot = (xcoor-0.1)*(sin(arg3))+(ycoor-0.1)*(cos(arg3))+0.1;
              return ycoor;//yrot+0.206;
            */
            arg3 = ARG3 * PI/180.0;
            xcoor =  0.55 * yChan + 0.252;
            ycoor = 0.55 * xChan + 0.206;
            yrot = (xcoor-0.252)*(sin(arg3))+(ycoor-0.206)*(cos(arg3))+0.206;
            return  yrot;
       
        }
    else if (module == 4)
        {
            /*
              arg4 = ARG4 * PI/180.0;
              xcoor =  -0.55 * xChan; - 0.1;
              ycoor = 0.55 * yChan; + 0.1;
              // abs   = sqrt(pow(xcoor,2)+pow(ycoor,2));
              //yrot = (xcoor+0.1)*(sin(arg4))+(ycoor-0.1)*(cos(arg4))+0.1;
              return ycoor;//yrot+0.229;
            */
      
            arg4 = ARG4 * PI/180.0;
            xcoor =  -0.55 * xChan - 0.069;
            ycoor = 0.55 * yChan + 0.229;
            yrot = (xcoor+0.069)*(sin(arg4))+(ycoor-0.229)*(cos(arg4))+0.229;
            return  yrot;
      
        }
    else {
        cerr << "ERROR: Wrong module number = " << module << endl;
        return -999.;
    }
}

void SetXChannelAdcData( int chSwap, int module, double* xData, double adcData );
void SetYChannelAdcData( int ch, int module, double* yData, double adcData );
int GetPedestal( TH1D* spect );

int main(int argc, char *argv[])
{
    SetShStyle( );
    
    FILE *fp;
    unsigned char header_buf[HEADER_SIZE];
    unsigned char data_buf[DATA_SIZE];
    int length;
    int trigger = -1;
    int nDataSet;
    short data;
    int c;
    int print_trigger_num = 0;
    unsigned int *int_p;
    unsigned short *short_p;
    int channel = -1;
    int module_num = -1;
    int mod_num;
    int print_header_in_hex = 1;

    double adcData[N_CHANNEL][N_SAMPLE];
    
    double clkCount[N_SAMPLE];
    double xData[N_XCHANNEL][N_SAMPLE];
    double yData[N_YCHANNEL][N_SAMPLE];

    int targetEvt = -100;
    String outputDir = "";
    if (argc == 1) {
        targetEvt = 1;
        outputDir = "output";
    }
    else if (argc == 2) {
        targetEvt = atoi( argv[1] );
        outputDir = "output";
    }
    else if (argc == 3) {
        targetEvt = atoi( argv[1] );
        outputDir = argv[2];
    }
    else {
        usage();
        exit(1);
    }

    // TApplication theApp("app", &argc, argv);

    TCanvas *c1 = new TCanvas("c1","c1 ",700,700);

    TH2F *image = new TH2F("image",";[mm];[mm]",200,-11,11,200,-11,11);
    TH1D *xspect[N_XCHANNEL];
    TH1D *yspect[N_YCHANNEL];
    int xmean[N_XCHANNEL];
    int ymean[N_YCHANNEL];

    // Channel uniformity
    TH1D *xChanUnif = new TH1D("x channel uniformity","",16,0.5,16.5);
    TH1D *yChanUnif = new TH1D("y channel uniformity","",16,0.5,16.5);

    std::vector< TH2F* > wfArray;
    for( int i = 0; i < N_CHANNEL; ++i ) {
        TH2F* pHist = new TH2F( Form( "waveform_%d", i ), Form( "waveform_%d", i ), 60, 0, 60, 4000, -1000, 1000 );
        wfArray.push_back( pHist );
    }

    TH1D* pHistChargeIntX = new TH1D( "histChargeIntX", "histChargeIntX", 100,0,20000 );
    TH1D* pHistChargeIntY = new TH1D( "histChargeIntY", "histChargeIntY", 100,0,20000 );

    TMultiGraph mgX;
    TMultiGraph mgY;
    std::vector< TGraph* > oneEvtWFArrayX;
    std::vector< TGraph* > oneEvtWFArrayY;
    
    
    for(int module=1; module<=4; module++){
  
        if ( module == 1 ){
            if ( (fp = fopen("m.dat", "r")) == NULL)
                err(1, "fopen");
        } else if ( module == 2 ){
            if ( (fp = fopen("s.dat", "r")) == NULL)
                err(1, "fopen");
        } else if ( module == 3 ){
            if ( (fp = fopen("s2.dat", "r")) == NULL)
                err(1, "fopen");
        } else if ( module == 4 ){
            if ( (fp = fopen("s3.dat", "r")) == NULL)
                err(1, "fopen");
        }
    
        for(int i = 0; i < N_XCHANNEL; i++)
            {
                xspect[i] = new TH1D("" , Form("X-strip-ch%d", i+1), 4096, 0 , 4095);
                yspect[i] = new TH1D("" , Form("Y-strip-ch%d", i+1), 4096, 0 , 4095);

                xmean[i] = 0;
                ymean[i] = 0;
            }
#ifdef BASELINE1
        ///////////////////Determin pedestal begin//////////////////////
        for (int t=0 ; t < 50000; t++ ) {
            ////////////HEADER////////////////////
    
            int nRead = fread(header_buf, 1, HEADER_SIZE, fp);
            if (nRead == 0) {
                if (feof(fp)) {
                    break;
                }
                else if (ferror(fp)) {
                    err(1, "fread error");
                }
            }
            if (nRead != HEADER_SIZE) {
                //errx(1, "HEADER: short read: %d bytes (should be %d bytes)",
                //   nRead, HEADER_SIZE);
                cerr << "WARNING: HEADER short read: " << nRead
                     << " bytes (should be " << HEADER_SIZE << " bytes)" << endl;
                break;
            }
            int_p = (unsigned int *)&header_buf[8];
            length = ntohl(*int_p);
            int_p = (unsigned int *)&header_buf[12];
            trigger = ntohl(*int_p);
            mod_num = header_buf[7];
            if (print_header_in_hex) {
                //printf("#### ");
                for (int i = 0; i < 15; i++) {
                    //printf("%02x ", header_buf[i]);
                    if ((i + 1) % 4 == 0) {
                        //printf("| ");
                    }
                }
                //prinaf("%02x\n", header_buf[15]);
            }
    
            //if (trigger<=100)
            ////for pedestal trigger number
            // printf("Trigger = %d  Length = %d \n",trigger,length); 
    
    
            ///////////////DATA////////////////
            nRead = fread(data_buf, 1, length, fp);
    
            if (nRead == 0) {
                if (feof(fp)) {
                    break;
                }
                else if (ferror(fp)) {
                    err(1, "fread error");
                }
            }
            if (nRead != length) {
                //errx(1, "DATA: short read: %d bytes (should be %d bytes)",
                //   nRead, length);
                cerr << "\nWARNING: DATA short read: " << nRead
                     << " bytes (should be " << length << " bytes)" << endl;
                break;
            }

            nDataSet = length / 2 / N_CHANNEL;
    
            for (int i = 0; i < nDataSet; i++) {
                for (int ch = 0; ch < N_CHANNEL; ch ++) {
                    int pos = 2*N_CHANNEL*i + 2*ch;
                    short_p = (unsigned short *)&data_buf[pos];
                    data = ntohs(*short_p);
                    //printf("%d %d %d %d", mod_num, ch, trigger, data);

                    adcData[ch][i] = ntohs(*short_p);
                    //printf("   Channel=%d Set=%d Data= %d\n", ch, i, adcData[ch][i]);
                }
            }
            //////////Dead channel processing//////////////////
            for (int i = 0; i < nDataSet; i++){
                for (int ch = 0; ch < N_YCHANNEL; ch++){
                    SetYChannelAdcData( ch, module, &yData[ch][i], adcData[ch][i] );
                    yspect[ch] -> Fill(yData[ch][i]);
                    //cout << "Y  " << ch << "  " << i << "= " << xData[ch][i] << endl;

                }
                for (int ch = N_YCHANNEL; ch < N_CHANNEL; ch++){	
                    int chSwap = ch;  // Channel swap for ch16-31
	  
                    if (ch%2==0)
                        chSwap = ch + 1;
                    else
                        chSwap = ch - 1;

                    chSwap = N_CHANNEL - chSwap - 1;
                    SetXChannelAdcData( chSwap, module, &xData[chSwap][i], adcData[ch][i] );
                    xspect[chSwap] -> Fill(xData[chSwap][i]);
                }
                clkCount[i] = i;
            }
            /*
              for (int ch = 0; ch < N_YCHANNEL; ch++)
              {
              for (int i = 0; i < nDataSet; i++)
              {
              cout << "X  " << ch << "  " << i << "= " << xData[ch][i] << endl;
              cout << "Y  " << ch << "  " << i << "= " << yData[ch][i] << endl;
              }
              }
            */
            //  int press_key = getchar();
        }
  
        //Get pedestal
        for(int i = 0; i < N_XCHANNEL; i++ )
            {
                xmean[i] = GetPedestal(xspect[i]);
                ymean[i] = GetPedestal(yspect[i]);
                cout << "ch" << i+1 << " x:" << xmean[i] << " y:" << ymean[i] << endl;
            }
        ///////////////////////Determin pedestal end///////////////////   


        //////////////////////Data anarysis////////////////////
        fseek(fp, 0, SEEK_SET); //return to top of read file
#endif

        ////deta analysis////
        for (  int t = 0 ; ; t++ ){
            // for (int t = 0; t < 500000 ; t++ ) {  
      
            xChanUnif->Reset();
            yChanUnif->Reset();
     
            ////////////HEADER////////////////////
            int nRead = fread(header_buf, 1, HEADER_SIZE, fp);
            if (nRead == 0) {
                if (feof(fp)) {
                    break;
                }
                else if (ferror(fp)) {
                    err(1, "fread error");
                }
            }
            if (nRead != HEADER_SIZE) {
                //errx(1, "HEADER: short read: %d bytes (should be %d bytes)",
                //   nRead, HEADER_SIZE);
                cerr << "WARNING: HEADER short read: " << nRead
                     << " bytes (should be " << HEADER_SIZE << " bytes)" << endl;
                break;
            }
            int_p = (unsigned int *)&header_buf[8];
            length = ntohl(*int_p);
            int_p = (unsigned int *)&header_buf[12];
            trigger = ntohl(*int_p);
            mod_num = header_buf[7];
            if (print_header_in_hex) {
                //printf("#### ");
                for (int i = 0; i < 15; i++) {
                    //printf("%02x ", header_buf[i]);
                    if ((i + 1) % 4 == 0) {
                        //printf("| ");
                    }
                }
                //printf("%02x\n", header_buf[15]);
            }
            //if (trigger<=100)
            if(trigger %50000 == 0)
                printf("Trigger = %d  Length = %d module =%d \n",trigger,length,module);

            ///////////////DATA2////////////////
      
            nRead = fread(data_buf, 1, length, fp);
            if (nRead == 0) {
                if (feof(fp)) {
                    break;
                }
                else if (ferror(fp)) {
                    err(1, "fread error");
                }
            }
            if (nRead != length) {
                //errx(1, "DATA: short read: %d bytes (should be %d bytes)",
                //   nRead, length);
                cerr << "\nWARNING: DATA short read: " << nRead
                     << " bytes (should be " << length << " bytes)" << endl;
                break;
            }
        
            nDataSet = length / 2 / N_CHANNEL;
            // if (trigger<=100)
            //     cout << "  Sample = " << nDataSet << endl;
            // if (trigger==100)
            //     cout << "Skip display..." << endl;
      
            for (int i = 0; i < nDataSet; i++) {
                for (int ch = 0; ch < N_CHANNEL; ch ++) {
                    int pos = 2*N_CHANNEL*i + 2*ch;
                    short_p = (unsigned short *)&data_buf[pos];
                    data = ntohs(*short_p);
                    //printf("%d %d %d %d", mod_num, ch, trigger, data);

                    adcData[ch][i] = ntohs(*short_p);
                    //printf("   Channel=%d Set=%d Data= %d\n", ch, i, adc_data[ch][i]);
                }
            }
      
            //////////Dead channel processing2//////////////////   

            // cout << "nDataSet = " << nDataSet << endl;
            for (int i = 0; i < nDataSet; i++){
                for (int ch = 0; ch < N_YCHANNEL; ch++){
                    SetYChannelAdcData( ch, module, &yData[ch][i], adcData[ch][i] );
                    // wfArray.at( ch )->Fill( i, adcData[ch][i] );
                }
	
                for (int ch = N_YCHANNEL; ch < N_CHANNEL; ch++){	
                    int chSwap = ch;  // Channel swap for ch16-31
	  
                    if (ch%2==0)
                        chSwap = ch + 1;
                    else
                        chSwap = ch - 1;
	  
                    chSwap = N_CHANNEL - chSwap - 1;
                    SetXChannelAdcData( chSwap, module, &xData[chSwap][i], adcData[ch][i] );
                    // wfArray.at( ch )->Fill( i, adcData[ch][i] );
                }
                clkCount[i] = i;
            }

            //zero suppression
#ifdef BASELINE1
            for( int i = 0; i <  N_XCHANNEL; i++)
                {
                    for( int j = 0; j < nDataSet; j++)
                        {
                            if(xmean[i] == 0)
                                {
                                    xData[i][j] = -999;
                                    yData[i][j] -= ymean[i];		  
                                }
                            else if(ymean[i] == 0)
                                {
                                    xData[i][j] -= xmean[i];	  
                                    yData[i][j] = 999;
                                }
                            else
                                {
                                    xData[i][j] -= xmean[i];
                                    yData[i][j] -= ymean[i];
                                }
                        }
                }
#endif
#ifdef BASELINE2
            double chargeIntX = 0.0, chargeIntY = 0.0;
            for (int ch = 0; ch < N_XCHANNEL; ch++){
                double xbaseline = 0;
                double ybaseline = 0;
                for (int i = BASELINE1_INI; i <= BASELINE1_END; i++)
                    xbaseline += xData[ch][i];
                for (int i = BASELINE2_INI; i <= BASELINE2_END; i++)
                    ybaseline += yData[ch][i];
                xbaseline /=(BASELINE1_END - BASELINE1_INI + 1);
                ybaseline /=(BASELINE2_END - BASELINE2_INI + 1);
                for (int i = 0; i < nDataSet; i++){
                    xData[ch][i] -= xbaseline;
                    yData[ch][i] -= ybaseline;
                    wfArray.at( ch )->Fill( i, xData[ch][i] );
                    wfArray.at( ch+N_XCHANNEL )->Fill( i, yData[ch][i] );
                    chargeIntX += xData[ch][i];
                    chargeIntY += yData[ch][i];
                }
            }
            pHistChargeIntX->Fill( chargeIntX );
            pHistChargeIntY->Fill( chargeIntY );

            // create one event waveform
            if( module == 1 && t == targetEvt ) {
                for (int ch = 0; ch < N_XCHANNEL; ch++){
                    // if( ch != 0 ) break;
                    double clkArr[60] = { };
                    double adcArrX[60] = { };
                    double adcArrY[60] = { };
                    
                    for (int i = 0; i < nDataSet; i++){
                        clkArr[i] = i;
                        adcArrX[i] = xData[ch][i] + static_cast< double >( ch ) * 200.0;
                        adcArrY[i] = yData[ch][i] + static_cast< double >( ch ) * 200.0;
                    }

                    TGraph* pGraphX = new TGraph( 60, clkArr, adcArrX );
                    TGraph* pGraphY = new TGraph( 60, clkArr, adcArrY );
                    pGraphX->SetMarkerSize( 1 );
                    pGraphY->SetMarkerSize( 1 );

                    pGraphX->SetMarkerColor( kPink + ch - 8);
                    pGraphY->SetMarkerColor( kBlue + ch - 8);

                    pGraphX->SetMarkerStyle( 7 );
                    pGraphY->SetMarkerStyle( 7 );
                    
                    mgX.Add( pGraphX );
                    mgY.Add( pGraphY );
                }
            }
#endif
    
            //////////Image Histogram//////////////////

            double xPos, yPos;
            double xChan = 0, yChan = 0;
            double xmean = 0, ymean = 0;
            double xsigma = 0, ysigma = 0;
            double xChargeSum = 0, yChargeSum = 0;
            double xChargeSum1 = 0, yChargeSum1 = 0;
            double xMaximum = 300, yMaximum = 300;
            int nx = 0, ny = 0;
            int nx1 = 0, ny1 = 0;
            int nx2 = 0, ny2 = 0;
            double xCharge1[N_XCHANNEL], yCharge1[N_YCHANNEL];
            double xCharge2[N_XCHANNEL], yCharge2[N_YCHANNEL];
            double SIGNAL_THR, CHANNEL_THR,xSIGMA,ySIGMA;
      
            if( module == 1 ){
                SIGNAL_THR = 23;
                CHANNEL_THR = 300;
                xSIGMA = 1.365, ySIGMA = 1.372;
            }
      
            if( module == 2 ){
                SIGNAL_THR = 10;
                CHANNEL_THR = 250;
                xSIGMA = 1.430, ySIGMA = 1.421;
            }
      
            if( module == 3 ){
                SIGNAL_THR = 12;
                CHANNEL_THR = 100;
                xSIGMA = 1.391, ySIGMA = 1.413;
            }
      
            if( module == 4 ){
                SIGNAL_THR = 7;
                CHANNEL_THR = 500;
                xSIGMA = 1.467, ySIGMA = 1.483;
            }
            /////////////////////////0209 done//////////////////
            for (int ch = 0; ch < N_XCHANNEL; ch++){
                double xCharge = 0, yCharge = 0;
                for (int i = SIGNAL_INI; i <= SIGNAL_END; i++){

                    if (xData[ch][i] >= SIGNAL_THR)
                        xCharge += xData[ch][i];
                    if (yData[ch][i] <= -1 * SIGNAL_THR)
                        yCharge -= yData[ch][i];  // y-signal is negative
                }

                xCharge1[ch] = xCharge;
                yCharge1[ch] = yCharge;
                /*
                  cout << "xCharge " << ch << " = " << xCharge1[ch] << endl;
                  cout << "yCharge " << ch << " = " << yCharge1[ch] << endl;
                  int press_key = getchar();
                */
            }
  
  
      	
            for (int ch = 0; ch < N_XCHANNEL; ch++){
                if ( xCharge1[ch-1] >= CHANNEL_THR || xCharge1[ch+1] >= CHANNEL_THR
                     || xCharge1[ch] >= CHANNEL_THR )
                    xCharge2[ch] = xCharge1[ch];
                else
                    xCharge2[ch] = 0;
                if ( yCharge1[ch-1] >= CHANNEL_THR || yCharge1[ch+1] >= CHANNEL_THR
                     || yCharge1[ch] >= CHANNEL_THR )
                    yCharge2[ch] = yCharge1[ch];
                else
                    yCharge2[ch] = 0;
      
                if ( ch == 0 ){
                    if ( xCharge1[ch+1] > CHANNEL_THR || xCharge1[ch] > CHANNEL_THR ){
                        xCharge2[ch] = xCharge1[ch];
                    }else
                        xCharge2[ch] = 0;
                    if ( yCharge1[ch+1] > CHANNEL_THR || yCharge1[ch] > CHANNEL_THR ){
                        yCharge2[ch] = yCharge1[ch];
                    }else
                        yCharge2[ch] = 0;
                }
                if ( ch == 15 ){
                    if ( xCharge1[ch-1] > CHANNEL_THR || xCharge1[ch] > CHANNEL_THR )
                        xCharge2[ch] = xCharge1[ch];
                    else
                        xCharge2[ch] = 0;
                    if ( yCharge1[ch-1] > CHANNEL_THR || yCharge1[ch] > CHANNEL_THR )
                        yCharge2[ch] = yCharge1[ch];
                    else
                        yCharge2[ch] = 0;
                }
      
                if ( xCharge2[ch] > xMaximum )
                    xMaximum = xCharge2[ch];
                if ( yCharge2[ch] > yMaximum )
                    yMaximum = yCharge2[ch];
      
                xChanUnif->Fill( ch+1, xCharge2[ch]);
                yChanUnif->Fill( ch+1, yCharge2[ch]);
            }
  
            for (int ch = 0; ch < N_XCHANNEL; ch++){
                xChan += ch * xCharge2[ch];
                xChargeSum += xCharge2[ch];
                yChan += ch * yCharge2[ch];
                yChargeSum += yCharge2[ch];
            }
      
            for (int ch = 0; ch < N_XCHANNEL; ch++){
                if (xCharge2[ch] != 0){
                    nx1 = ch+1;
                    break;
                }
            }

            for (int ch = 0; ch < N_XCHANNEL; ch++){
                if ( yCharge2[ch] != 0){
                    ny1 = ch+1;
                    break;
                }
            }
    
            for (int ch = 0; ch < N_XCHANNEL; ch++){
                if ( xCharge2[ch] != 0)
                    nx2 = ch+1;
                else if ((ch == 15 && xCharge2[ch] >= CHANNEL_THR)||(ch == 15 && xCharge2[ch] <= CHANNEL_THR && xCharge2[ch-1] >= CHANNEL_THR))
                    nx2 = ch+1;
      
                if ( yCharge2[ch] != 0)
                    ny2 = ch+1;
                else if ((ch == 15 && yCharge2[ch] >= CHANNEL_THR)||(ch == 15 && yCharge2[ch] <= CHANNEL_THR && yCharge2[ch-1] >= CHANNEL_THR))
                    ny2 = ch+1;
            }
    
            nx = nx2 - nx1;
            ny = ny2 - ny1;
    
            if (nx != 0)
                nx += 1;
            if (ny != 0)
                ny += 1;
   
            //////////Gaus fit///////////   
            if(module == 3)
                {
                    if ( nx >= NCHAN_THR_3X && ny >= NCHAN_THR && nx <= NCHAN_THR_3X1 && ny <= NCHAN_THR1 ){
	    
                        xChan /= xChargeSum;
                        yChan /= yChargeSum;
	    
	    
                        TF1 *fx = new TF1("fx","gaus");
                        fx->SetParameters(xMaximum,xChan,xSIGMA);
                        xChanUnif->Fit("fx","Q","",0.5,16.5);
	    
                        TF1 *fy = new TF1("fy","gaus");
                        fy->SetParameters(yMaximum,yChan,ySIGMA);
                        yChanUnif->Fit("fy","Q","",0.5,16.5);
	    
	    
                        xmean = fx->GetParameter(1);
                        ymean = fy->GetParameter(1);
                        xsigma = fx->GetParameter(2);
                        ysigma = fy->GetParameter(2);
	    
                        if ( xmean <= mean1 && ymean <= mean1 && xmean >= mean &&
                             ymean >= mean && xsigma <= xSIGMA +0.22 && ysigma <= ySIGMA +0.22 && xsigma >= xSIGMA -0.22 && ysigma >= ySIGMA -0.22){
                            // if(module == 3 && xChargeSum > 2000 && yChargeSum >2000){
                            xPos = xCoordinate( module, xmean -0.5 , ymean -0.5 );
                            yPos = yCoordinate( module, xmean -0.5 , ymean -0.5 );
                            image->Fill( xPos, yPos );
                            //}
                        }
	    
                    }else {
                        xPos = -99.;
                        yPos = -99.;
                    }
                }
            else{
	
                if ( nx >= NCHAN_THR && ny >= NCHAN_THR && nx <= NCHAN_THR1 && ny <= NCHAN_THR1 ){
	  
                    xChan /= xChargeSum;
                    yChan /= yChargeSum;
	  
	  
                    TF1 *fx = new TF1("fx","gaus");
                    fx->SetParameters(xMaximum,xChan,xSIGMA);
                    xChanUnif->Fit("fx","Q","",0.5,16.5);
	  
                    TF1 *fy = new TF1("fy","gaus");
                    fy->SetParameters(yMaximum,yChan,ySIGMA);
                    yChanUnif->Fit("fy","Q","",0.5,16.5);
	  
                    xmean = fx->GetParameter(1);
                    ymean = fy->GetParameter(1);
                    xsigma = fx->GetParameter(2);
                    ysigma = fy->GetParameter(2);
	  
                    if ( xmean <= mean1 && ymean <= mean1 && xmean >= mean &&
                         ymean >= mean && xsigma <= xSIGMA +0.22 && ysigma <= ySIGMA +0.22 && xsigma >= xSIGMA -0.22 && ysigma >= ySIGMA -0.22){
                        xPos = xCoordinate( module, xmean -0.5 , ymean -0.5 );
                        yPos = yCoordinate( module, xmean -0.5 , ymean -0.5 );
                        image->Fill( xPos, yPos );
                    }
	  
                }else {
                    xPos = -99.;
                    yPos = -99.;
                }
            }
        }
        fclose(fp);
    }

    //////////Draw Graph//////////////////

    const Int_t NRGBs = 5;
    const Int_t NCont = 255;

    Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
    Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 1.00 };
    Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
    Double_t blue[NRGBs]  = { 0.80, 1.00, 0.12, 0.00, 0.00 };
    TColor::CreateGradientColorTable( NRGBs, stops, red, green, blue, NCont );
    gStyle->SetNumberContours( NCont );
    gPad->SetRightMargin(0.2);
    
    if (DRAW_MODE == 2){
        // gROOT->Reset();
        //c1->SetLogz(1);
        // gStyle->SetPalette(93);//93
        image->SetMaximum(60);
        image->SetStats(0);
        image->Draw("colz");
        //image->Draw("box");
        //image->Draw("scat");
        //image->Draw("lego");
        // c1->Update();
        c1->Print("190209.pdf");
        c1->SetLogz(1);
    }

    TCanvas cvs2D( "cvs2D", "cvs2D", 1600, 800 );
    gStyle->SetNumberContours( NCont );
    cvs2D.SetLogz(1);
    cvs2D.Divide( 8, 4 );

    for( int i = 0; i < N_CHANNEL; ++i ) {
        cvs2D.cd( i+1 );
        gPad->SetRightMargin(0.2);
        gPad->SetLogz(1);
        wfArray.at( i )->Draw( "colz" );
    }
    cvs2D.Update( );
    cvs2D.SaveAs( Form( "%s/waveformAll.png", outputDir.c_str( ) ) );
    cvs2D.SaveAs( Form( "%s/waveformAll.pdf", outputDir.c_str( ) ) );
    
    TCanvas cvs1D( "cvs1D", "cvs1D", 800, 600 );
    // cvs1D.SetLogy( 1 );

    pHistChargeIntX->SetLineColor( kRed );
    pHistChargeIntY->SetLineColor( kBlue );

    pHistChargeIntX->GetXaxis()->SetTitle( "Sum of ADC counts (pedestal sduabtracted)" );
    pHistChargeIntX->GetYaxis()->SetTitle( "Events" );
    
    pHistChargeIntX->Draw( );
    pHistChargeIntY->Draw( "same" );
    cvs1D.SaveAs( Form( "%s/energy.png", outputDir.c_str( ) ) );
    cvs1D.SaveAs( Form( "%s/energy.pdf", outputDir.c_str( ) ) );

    mgX.Draw( "AP" );
    cvs1D.SaveAs( Form( "%s/oneEvtWFX_%d.png", outputDir.c_str( ), targetEvt ) );
    cvs1D.SaveAs( Form( "%s/oneEvtWFX_%d.pdf", outputDir.c_str( ), targetEvt ) );

    mgY.Draw( "AP" );
    cvs1D.SaveAs( Form( "%s/oneEvtWFY_%d.png", outputDir.c_str( ), targetEvt ) );
    cvs1D.SaveAs( Form( "%s/oneEvtWFY_%d.pdf", outputDir.c_str( ), targetEvt ) );
    
    return 0;
}


///////function////////
void SetXChannelAdcData( int chSwap, int module, double* xData, double adcData )
{
    switch( module )
        {
        case 0:
        case 1:
            if(chSwap == DEADCHAN_1)
                *xData = -999;
            else
                *xData = adcData;
            break;
        case 2:
            if(chSwap == DEADCHAN_2)
                *xData = -999;
            else
                *xData = adcData;
            break;
        case 3:
            if(chSwap == DEADCHAN_3 || chSwap == DEADCHAN_4 )
                *xData = -999;
            else
                *xData = adcData;
            break;
        case 4:
            *xData = adcData;
            break;
    
        default:
            *xData = -999;
            break;
        }
}

void SetYChannelAdcData( int ch, int module, double* yData, double adcData )
{
    switch( module )
        {
        case 0:
        case 1:    
        case 2:
        case 3: 
            *yData = adcData;
            break;
        case 4:
            if(ch == DEADCHAN_5)
                *yData = 999;
            else
                *yData = adcData;
            break;
    
        default:
            *yData = 999;
            break;
        }
}

int GetPedestal( TH1D* spect )
{
    double *tmpData;
    int tmpMax, tmpMaxHalf;
    int halfL, halfR;
    bool isOver;

    tmpData = spect -> GetArray();
    tmpMaxHalf = spect -> GetMaximum() / 2;
    isOver = false;
    for(int j = 0; j < spect -> GetSize(); j++)
        {
            if(tmpData[j] == spect -> GetMaximum())
                {
                    isOver = true;
                }
	  
            // left
            if( isOver == false )
                {
                    if(halfL == 0)
                        {
                            if(tmpData[j] >= tmpMaxHalf)
                                {
                                    halfL = j;
                                }
                        }
                    else if(tmpData[j] < tmpMaxHalf)
                        {
                            halfL = 0;
                        }
                }
            // right
            else
                {
                    if(tmpData[j-1] >= tmpMaxHalf)
                        {
                            if(tmpData[j] < tmpMaxHalf)
                                {
                                    halfR = j - 1;
                                }
                        }
                }
        }
    TF1 *fx = new TF1("fx","gaus");
    spect -> Fit("fx","Q","", halfL, halfR);
  
    return fx -> GetParameter(1);
    delete fx;
}
