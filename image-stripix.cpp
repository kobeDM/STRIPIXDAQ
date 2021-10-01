#include <iostream>
using namespace std;

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

#define HEADER_SIZE (4*4)
#define DATA_SIZE (1024*1024)
#define N_XCHANNEL 16
#define N_YCHANNEL 16
#define N_CHANNEL 32

#define N_SAMPLE 60 
#define BASELINE1_INI 0 
//#define BASELINE1_END 14
#define BASELINE1_END 7
#define BASELINE2_INI 0
//#define BASELINE2_END 14
#define BASELINE2_END 7
#define BASELINE_SUBTRACT 1

//#define SIGNAL_INI 15
#define SIGNAL_INI 10
#define SIGNAL_END 60

#define mean 0.5
#define mean1 16.5

#define NCHAN_THR 3
#define NCHAN_THR1 9

#define TOTAL 553000

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


#define DEADCHAN_1 22
#define DEADCHAN_2 30
#define DEADCHAN_3 21
#define DEADCHAN_4 3


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
  if (module == 1)
    return  0.55 * yChan + 0.1;
  else if (module == 2)
    return  0.55 * xChan + 0.1;
  else if (module == 3)
    return -0.55 * xChan - 0.1;
  else if (module == 4)
    return -0.55 * yChan - 0.1;
  else {
    cerr << "ERROR: Wrong module number = " << module << endl;
    return -999.;
  }
}

double yCoordinate( int module, double xChan, double yChan ){
  if (module == 1)
    return  0.55 * xChan + 0.1;
  else if (module == 2)
    return -0.55 * yChan - 0.1;
  else if (module == 3)
    return  0.55 * yChan + 0.1;
  else if (module == 4)
    return -0.55 * xChan - 0.1;
  else {
    cerr << "ERROR: Wrong module number = " << module << endl;
    return -999.;
  }
}

int main(int argc, char *argv[])
{
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

  // Option arguments
  while ( (c = getopt(argc, argv, "c:m:t")) != -1) {
    switch (c) {
    case 'c':
      channel = strtol(optarg, NULL, 0);
      break;
    case 'm':
      module_num = strtol(optarg, NULL, 0);
      break;
    case 't':
      print_trigger_num = 1;
      break;
    default:
      break;
    }
  }

  argc -= optind;
  argv += optind;
  
  //if (argc >= 2) {
  if (argc >= 1) {
    usage();
    exit(1);
  }
  

  TApplication theApp("app", &argc, argv);

  TCanvas *c1 = new TCanvas("c1","c1 ",800,800);

  TH2F *image = new TH2F("image","",200,-11,11,200,-11,11);

  // Channel uniformity
  TH1D *xChanUnif = new TH1D("x channel uniformity","",16,0.5,16.5);
  TH1D *yChanUnif = new TH1D("y channel uniformity","",16,0.5,16.5);
  
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
    
    //for ( int t = 0; t < TOTAL; t++ ){　
    for ( ; ; ) {  
      
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
	printf("Trigger = %d  Length = %d module =%d \n",trigger,length,module);

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
      if (trigger<=100)
	cout << "  Sample = " << nDataSet << endl;
      if (trigger==100)
	cout << "Skip display..." << endl;
      
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


      if (BASELINE_SUBTRACT){
	for (int ch = 0; ch < N_CHANNEL; ch++){
	  double baseline = 0;
	  for (int i = BASELINE1_INI; i <= BASELINE1_END; i++)
	    baseline += adcData[ch][i];
	  for (int i = BASELINE2_INI; i <= BASELINE2_END; i++)
	    baseline += adcData[ch][i];
	  baseline /=
	    (BASELINE1_END - BASELINE1_INI +BASELINE2_END - BASELINE2_INI +2);
	  for (int i = 0; i < nDataSet; i++){
	    adcData[ch][i] -= baseline;
	  }
	}
      }
      
      
      
      //////////Dead channel processing//////////////////   
      
      for (int i = 0; i < nDataSet; i++){
	for (int ch = 0; ch < N_YCHANNEL; ch++){
	  if ( module == 4 && ch != DEADCHAN_4 )
	    yData[ch][i] = adcData[ch][i];
	  else if ( module == 1 ||  module == 2 ||  module == 3 )
	    yData[ch][i] = adcData[ch][i];
	  else
	    yData[ch][i] = 999;
	
	}
      
	for (int ch = N_YCHANNEL; ch < N_CHANNEL; ch++){
	
	  int chSwap = ch;  // Channel swap for ch16-31
	  
	  if (ch%2==0)
	    chSwap = ch + 1;
	  else
	    chSwap = ch - 1;
	  chSwap = N_CHANNEL - chSwap - 1;
	  
	  if ( module == 1 && ch != DEADCHAN_1 )
	    xData[chSwap][i] = adcData[ch][i];
	  else if ( module == 2 && ch != DEADCHAN_2 )
	    xData[chSwap][i] = adcData[ch][i];
	  else if ( module == 3 && ch != DEADCHAN_3 )
	    xData[chSwap][i] = adcData[ch][i];
	  else if ( module == 4 )
	    xData[chSwap][i] = adcData[ch][i];
	  else
	    xData[chSwap][i] = -999;
	  
	}
	clkCount[i] = i;
      }
   

    
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
	SIGNAL_THR = 25;
	CHANNEL_THR = 300;
	xSIGMA = 1.338, ySIGMA = 1.357;
      }
      
      if( module == 2 ){
	SIGNAL_THR = 20;
	CHANNEL_THR = 450;
	xSIGMA = 1.387, ySIGMA = 1.375;
      }
      
      if( module == 3 ){
	SIGNAL_THR = 12;
	CHANNEL_THR = 200;
	xSIGMA = 1.389, ySIGMA = 1.425;
      }
      
      if( module == 4 ){
	SIGNAL_THR = 15;
	CHANNEL_THR = 500;
	xSIGMA = 1.409, ySIGMA = 1.415;
      }

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

    
      if ( nx >= NCHAN_THR && ny >= NCHAN_THR && nx <= NCHAN_THR1 && ny <= NCHAN_THR1 ){
      
	xChan /= xChargeSum;
	yChan /= yChargeSum;

	
	TF1 *fx = new TF1("fx","gaus");
	fx->SetParameters(xMaximum,xChan,xSIGMA);
	xChanUnif->Fit("fx","","",0.5,16.5);

	TF1 *fy = new TF1("fy","gaus");
	fy->SetParameters(yMaximum,yChan,ySIGMA);
	yChanUnif->Fit("fy","","",0.5,16.5);
      

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
      
      } else {
	xPos = -99.;
	yPos = -99.;
      }
      
	
    }
    fclose(fp);
  }
  

  //////////Draw Graph//////////////////
    
  if (DRAW_MODE == 2){
    gROOT->Reset();
    //c1->SetLogz(1);
    gStyle->SetPalette(93);
    image->SetMaximum(50);
    image->Draw("colz");
    //image->SetStats(0);
    //image->Draw("box");
    //image->Draw("scat");
    //image->Draw("lego");
    c1->Update();
    c1->Print("imaging.pdf");
    int press_key = getchar();
  }

  return 0;
}
