#include <err.h>
#include <unistd.h>
#include <iostream>
using namespace std;

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <TString.h>
#include <math.h>
#include <TApplication.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TMarker.h>
#include <TArrayD.h>
#define HEADER_SIZE (4*4)
#define DATA_SIZE (1024*1024)
#define N_XCHANNEL 16
#define N_YCHANNEL 16
#define N_CHANNEL 32

#define USE_DEADCHAN_BD 0
//--^ turn on (1) for data160216/data33 and before

#define N_SAMPLE 60
//#define N_SAMPLE 100
#define SIGNAL_INI 15
#define SIGNAL_END 60
#define SIGNAL_XTHR -60
#define SIGNAL_YTHR 60
#define mean 0.5
#define mean1 16.5
  
#define DRAW_MODE 1
//--^ 1: Draw events, 2: Charge
/* MODULE Layout
   +---+|||+
   | 4 | 3 |
   +---+---+
   | 2 | 1 |
   +---+---+
   
   Dead channel 
   MODULE1(m.dat):Y-9ch(8ch)
   MODULE2(s.dat):Y-1ch(0ch)
   MODULE3(s2.dat):Y-2ch...8ch,Y-12ch(11ch)
   MODULE4(s3.dat):X-4ch(29ch)

   MODULE0:Don't adapt
*/

#define DEADCHAN_1 8
#define DEADCHAN_2 0
#define DEADCHAN_3 11
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
/*
double xCoordinate( int module, double xChan, double yChan ){
  if (module == 1)
    return  0.55 * xChan + 0.35;
  else if (module == 2)
    return -0.55 * yChan - 0.35;
  else if (module == 3)
    return  0.55 * yChan + 0.35;
  else if (module == 4)
    return -0.55 * xChan - 0.35;
  else {
    cerr << "ERROR: Wrong module number = " << module << endl;
    return -999.;
  }
}

double yCoordinate( int module, double xChan, double yChan ){
  if (module == 1)
    return -0.55 * yChan - 0.35;
  else if (module == 2)
    return -0.55 * xChan - 0.35;
  else if (module == 3)
    return  0.55 * xChan + 0.35;
  else if (module == 4)
    return  0.55 * yChan + 0.35;
  else {
    cerr << "ERROR: Wrong module number = " << module << endl;
    return -999.;
  }
}
*/
#define BASELINE
//#define PRINT_FIT
#define PRINT_X_CH 0
#define PRINT_Y_CH 0


void SetXChannelAdcData( int ch, int module, double* yData, double adcData );
void SetYChannelAdcData( int chSwap, int module, double* yData, double adcData );
int GetPedestal( TH1D* spect );

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
  double xCharge = 0,yCharge = 0;
  double adcData[N_CHANNEL][N_SAMPLE];
    
  double clkCount[N_SAMPLE];
  double xData[N_XCHANNEL][N_SAMPLE];
  double yData[N_YCHANNEL][N_SAMPLE];

  int module;

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
  
  if (argc >= 2) {
    usage();
    exit(1);
  }
  
  cout << "argc = " << argc << endl;

  if (argc == 1){
    if ( (fp = fopen(argv[0], "r")) == NULL) {
      err(1, "fopen");
    }

    if(strcmp(argv[0], "m.dat") == 0)
      {
	module = 1;
      }
    else if(strcmp(argv[0], "s.dat") == 0)
      {
	module = 2;
      }
    else if(strcmp(argv[0], "s2.dat") == 0)
      {
	module = 3;
      }
    else if(strcmp(argv[0], "s3.dat") == 0)
      {
	module = 4;
      }
    else
      {
	module = 0;
      }
  } else if(argc == 0){
    fp = stdin;
  }else{
    exit(0);
  }

  TApplication theApp("app", &argc, argv);

  TCanvas *c1 = new TCanvas("c1","c1 ",1000,800);
  TH1D *xcharge = new TH1D("","X-strip-charge",201,-100,100);
  TH1D *ycharge = new TH1D("","Y-strip-charge",201,-100,100);

  TH1D *xspect[N_XCHANNEL];
  TH1D *yspect[N_YCHANNEL];
  int xmean[N_XCHANNEL];
  int ymean[N_YCHANNEL];
#ifdef PRINT_FIT
  TH1D *xspectFull;
  TH1D *yspectFull;
#endif // PRINT_FIT

  for(int i = 0; i < N_XCHANNEL; i++)
    {
      xspect[i] = new TH1D("" , Form("X-strip-ch%d", i+1), 4096, 0 , 4095);
#ifdef PRINT_FIT
      if(i == PRINT_X_CH)
	{
	  xspectFull = new TH1D("" ,  Form("X-strip-ch%d", i+1), 4096, 0 , 4095);
	}
#endif // PRINT_FIT
      yspect[i] = new TH1D("" , Form("Y-strip-ch%d", i+1), 4096, 0 , 4095);
#ifdef PRINT_FIT
      if(i == PRINT_Y_CH)
	{
	  yspectFull = new TH1D("" , Form("Y-strip-ch%d", i+1), 4096, 0 , 4095);
	}
#endif // PRINT_FIT
      xmean[i] = 0;
      ymean[i] = 0;
    }

  for (int t=0 ; t < 100000; t++ ) {
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
#if 0
    if (trigger<=100)
      cout << "  Sample = " << nDataSet << endl;

    if (trigger==100)
      cout << "Skip display...\n" << endl;
#endif

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
      for (int ch = 0; ch < N_XCHANNEL; ch++){
	SetXChannelAdcData( ch, module, &xData[ch][i], adcData[ch][i] );
        xspect[ch] -> Fill(xData[ch][i]);
#ifdef PRINT_FIT
	if( ch == PRINT_X_CH )
	  {
	    xspectFull -> Fill(xData[ch][i]);
	  }
#endif // PRINT_FIT

      }

      for (int ch = N_YCHANNEL; ch < N_CHANNEL; ch++){	
	int chSwap = ch;  // Channel swap for ch16-31
	  
        if (ch%2==0)
	  chSwap = ch + 1;
	else
	  chSwap = ch - 1;

	chSwap = N_CHANNEL - chSwap - 1;
	SetYChannelAdcData( chSwap, module, &yData[chSwap][i], adcData[ch][i] );
        yspect[chSwap] -> Fill(yData[chSwap][i]);
#ifdef PRINT_FIT
	if( chSwap == PRINT_Y_CH)
	  {
	    yspectFull -> Fill(yData[chSwap][i]);
	  }
#endif //PRINT_FIT
      }
      clkCount[i] = i;
    }
  }
  //Get pedestal
  for(int i = 0; i < N_XCHANNEL; i++ )
    {
      xmean[i] = GetPedestal(xspect[i]);
      ymean[i] = GetPedestal(yspect[i]);
      cout << "ch" << i+1 << " x:" << xmean[i] << " y:" << ymean[i] << endl;
    }
  //spect destruct
#ifndef PRINT_FIT
  for( int i = 0; i < N_XCHANNEL; i++ )
    {
      delete xspect[i];
      delete yspect[i];
    }
#endif // PRINT_FIT
  // cout << "module:" << module << endl;
  //   clkCount[i] = i;

//////////Image Histogram//////////////////

#ifdef PRINT_FIT //ADC-distribution
  TF1 *fx = new TF1("fx","gaus");
  TF1 *fy = new TF1("fy","gaus");

  gROOT -> Reset();
  gStyle -> SetOptFit();
  c1 -> Clear();
  c1 -> SetLogy();
  c1 -> Divide(2,2);
  c1 -> cd(1);
  gPad -> SetLogy(1);
  fx -> SetLineColor(2);
  //xspect1 -> SetFillColor(kRed);
  xspectFull -> Draw();
  
  c1 -> cd(3);
  //gPad -> SetLogy(1);
  fx -> SetLineColor(2);
  xspect[PRINT_X_CH] -> GetYaxis() -> SetRangeUser( (xspect[PRINT_X_CH] -> GetMaximum() / 2), xspect[PRINT_X_CH] -> GetMaximum() +100000);
  xspect[PRINT_X_CH] -> GetXaxis() -> SetRangeUser(xmean[PRINT_X_CH] - 100, xmean[PRINT_X_CH] + 100);
  xspect[PRINT_X_CH] -> Draw();

  c1 -> cd(2);
  gPad -> SetLogy(1);
  fy -> SetLineColor(2);
  //xspectFull -> SetFillColor(kRed);
  yspectFull -> Draw();

  c1 -> cd(4);
  //gPad -> SetLogy(1);
  fy -> SetLineColor(2);
  yspect[PRINT_Y_CH] -> GetYaxis() -> SetRangeUser( (yspect[PRINT_Y_CH] -> GetMaximum() / 2), yspect[PRINT_Y_CH] -> GetMaximum() +100000);
  yspect[PRINT_Y_CH] -> GetXaxis() -> SetRangeUser(ymean[PRINT_Y_CH] - 100, ymean[PRINT_Y_CH] + 100);
  // yspect[PRINT_YCH] -> SetFillColor(kRed);
  yspect[PRINT_Y_CH] -> Draw();
  c1 -> Update();
  //c1 -> Print("dead-ch-Chip1-ch1.pdf");
  //c1 -> Print("pedestal111_chip1_ch4_unset_dead.pdf"); //write by myself
  int press_key = getchar();
#endif // PRINT_FIT

  ////data_analysis (each trigger)
  c1 -> Clear();
  c1 -> Divide(4,4);

  fseek(fp, 0, SEEK_SET); //return to top of read file
  for (int t=0 ; ; t++ ) {
////////////HEADER2////////////////////
    
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
     printf("Trigger = %d  Length = %d \n",trigger,length);
    
    
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
#if 0
    if (trigger<=100)
      cout << "  Sample = " << nDataSet << endl;
    if (trigger==100)
      cout << "Skip display...\n" << endl;
#endif

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
      for (int ch = 0; ch < N_XCHANNEL; ch++){
	SetXChannelAdcData( ch, module, &xData[ch][i], adcData[ch][i] );
      }

      for (int ch = N_YCHANNEL; ch < N_CHANNEL; ch++){	
	int chSwap = ch;  // Channel swap for ch16-31
	  
        if (ch%2==0)
	  chSwap = ch + 1;
	else
	  chSwap = ch - 1;

	chSwap = N_CHANNEL - chSwap - 1;
	SetYChannelAdcData( chSwap, module, &yData[chSwap][i], adcData[ch][i] );
      }
      clkCount[i] = i;
    }
#ifdef BASELINE //zero suppression
    for( int i = 0; i <  N_XCHANNEL; i++)
      {
	for( int j = 0; j < nDataSet; j++)
	  {
	    xData[i][j] -= xmean[i];
	    yData[i][j] -= ymean[i];
	  }
      }
#endif
 ///////determin peak clk//////
    //  int XpeakClk[N_XCHANNEL][8] , YpeakClk[N_YCHANNEL][8];
    int XpeakClk[7][N_XCHANNEL];
    int YpeakClk[7][N_YCHANNEL];
    double XpeakSignal[N_XCHANNEL] , YpeakSignal[N_YCHANNEL];
    double xData2[N_XCHANNEL][N_SAMPLE];
    double yData2[N_YCHANNEL][N_SAMPLE];
    double XchargeSignal = 0, YchargeSignal = 0;
  
    for( int i = 0; i < N_XCHANNEL; i++)
      {
	int c = 0;
	int d = 0;
	/////////XpeakClk////////////
	for(int j = 0; j < nDataSet; j++)
	  {
	    if(xData[i][j] < SIGNAL_XTHR)
	      {
		xData2[i][j] = xData[i][j];
	      }
	    else
	      xData2[i][j] = 0;
		//XpeakSignal[i] = xData[i][j];
		//XpeakClk[i] = j;	    
	  }
	for(int j = 0; j < nDataSet; j++)
	  {
	    if(j == 0)
	      {
		continue;
	      }
	    else if(xData2[i][j] < xData2[i][j-1] &&
		    xData2[i][j] < xData2[i][j+1]   )
	      {
		XpeakClk[c][i]= j;
		//	cout << "Xpeak" << c+1 <<' '<< i+1 << " = " << XpeakClk[c][i] << endl;
		c = c+1;
	      }
	  }
      	///////////YpeakClk////////////    
	for(int k = 0; k < nDataSet; k++)
	  {
	    if(yData[i][k] > SIGNAL_YTHR)
	      {
		yData2[i][k] = yData[i][k];
		//YpeakSignal[i] = yData[i][k];
		//YpeakClk[i] = k;
	      }
	    else
	      yData2[i][k] = 0;
	  }
	for(int j = 0; j < nDataSet; j++)
	  {
	    if(j == 0)
	      {
		continue;
	      }
	    else if(yData2[i][j] > yData2[i][j-1] &&
		    yData2[i][j] > yData2[i][j+1]   )
	      {
		YpeakClk[d][i]= j;
		//	cout << "Ypeak" << d+1 <<' '<< i+1 << " = " << YpeakClk[d][i] << endl;
		d = d+1;
	      }
	  }
      }/////pre
#if 0
	//////////PeakSelection//////////
	for(int j = 0; ; j++)
	  {
	    if((XpeakClk[j+1][i]-XpeakClk[j][i]) > 
      }

    //////////clock matching////////
      for(int i = 0; i < N_XCHANNEL; i++)
     {
#endif	
#if 0
	//cout << "XpeakClk" << i+1 << " = " << XpeakClk[i] << endl;
	//cout << "YpeakClk" << i+1 << " = " << YpeakClk[i] << endl;
	if(XpeakClk[i] == YpeakClk[i])
	  {
	    for(int j = 0; j< nDataSet; j++)
	      {
		xData2[i][j] = 0;
		yData2[i][j] = 0;
		for(int k = (XpeakClk[i]-2); k < (XpeakClk[i]+6); k++)
		  {
		    xData2[i][k] = xData[i][k];
		    yData2[i][k] = yData[i][k];
		  }
	      }
	  }
	else
	  {
	    for(int j = 0; j < nDataSet; j++)
	      {
		xData2[i][j] = 0;
		yData2[i][j] = 0;
	      }
	  }
#endif
	//}
	    
 ///////charge integral/////////
     for( int i = 0; i <  N_XCHANNEL; i++)
      {
	if(module == 4  && i == 3)
	continue;
	
	for( int j = 0; j < nDataSet; j++)
	  {
	    xcharge -> Fill(xData[i][j]);
	  }
      }
     for( int i = 0; i <  N_XCHANNEL; i++)
      {
	if((module == 1 && i == 8) || (module == 2 && i == 0) ||
	   (module == 3 && i == 11))
	continue;
	
	for( int j = 0; j < nDataSet; j++)
	  {
	    ycharge -> Fill(yData[i][j]);
	  }
      }
/////draw graph////
     if(DRAW_MODE == 1)
       {
	 ////make graphs
	 TGraph *xgraph[N_XCHANNEL];
	 TGraph *ygraph[N_YCHANNEL];
	 for (int i = 0; i < N_XCHANNEL; i++)
	   xgraph[i] = new TGraph();
	 for (int i = 0; i < N_YCHANNEL; i++)
	   ygraph[i] = new TGraph();
	 
	 gROOT ->Reset();
	 //Draw graphs
	 for (int ch = 0; ch < N_XCHANNEL; ch++) {
	   
	   for(int i = 0; i < nDataSet; i++){
#if 1
	     xgraph[ch]->SetPoint(i,clkCount[i],xData[ch][i]);
	     ygraph[ch]->SetPoint(i,clkCount[i],yData[ch][i]);
#endif
#if 0
	     xgraph[ch]->SetPoint(i,clkCount[i],xData2[ch][i]);
	     ygraph[ch]->SetPoint(i,clkCount[i],yData2[ch][i]);
#endif
	   }
	   xgraph[ch]->SetName(Form("xgraph%d",ch));
	   xgraph[ch]->SetTitle(Form("Chip%d Ch%d", module, ch+1));
	   //for zero_suppress
#ifdef BASELINE
	   xgraph[ch]->SetMinimum(-1000);
	   xgraph[ch]->SetMaximum(1000);
	   ygraph[ch]->SetMinimum(-1000);
	   ygraph[ch]->SetMaximum(1000);
#else
	   //low data
	   xgraph[ch]->SetMinimum(0);
	   xgraph[ch]->SetMaximum(4096);
	   ygraph[ch]->SetMinimum(0);
	   ygraph[ch]->SetMaximum(4096);
#endif   
	   xgraph[ch]->SetLineColor(2);
	   ygraph[ch]->SetLineColor(4);
	   
	   c1->cd(ch+1);
	   xgraph[ch] -> Draw("al");
	   ygraph[ch] -> Draw("SAME");
	 }
	 
	 c1 -> Update();//Canvas updata (needless "theApp.Run")
	 //if(t == 52){
	 // c1 -> Print ("THR0.pdf");
	 
	 int press_key = getchar();
       }
  }

  if(DRAW_MODE == 2)
    {
      gROOT -> Reset();
      gStyle -> SetOptFit();
      c1 -> Clear();
      
      c1->Divide(1,2);
      c1->cd(1);
      gPad -> SetLogy(1);
      xcharge -> Draw();

      c1->cd(2);
      gPad -> SetLogy(1);
      ycharge -> Draw();

      c1 -> Update();
      // c1 -> Print("Chip2-Charge-undead-re.pdf");
      int press_key = getchar();
    }

  return 0;
}

///////function////////
void SetXChannelAdcData( int ch, int module, double* xData, double adcData )
{
  switch( module )
    {
    case 0:
    case 1:
    case 2:
    case 3:
      *xData = adcData;
      break;

    case 4:
      if( ch == DEADCHAN_4)
	*xData = 999;
      else
	*xData = adcData;
      break;

    default:
      *xData = 999;
      break;
    }
}

void SetYChannelAdcData( int chSwap, int module, double* yData, double adcData )
{
  switch( module )
    {
    case 1:
      if( chSwap == DEADCHAN_1)
	*yData = -999;
      else
	*yData = adcData;
      break;
	    
    case 2:
      if( chSwap == DEADCHAN_2)
	*yData = -999;
      else
	*yData = adcData;
      break;
    case 3:
      if( chSwap == DEADCHAN_3)
	*yData = -999;
      else
	*yData = adcData;
      break;
	    
    case 0:
    case 4:
      *yData = adcData;
      break;

    default:
      *yData = -999;
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

