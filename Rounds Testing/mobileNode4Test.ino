/* 
 *  File        : mobileNode4Test.ino
 *  Description : Implements the R.I.D.E localization algorithm. 
 *                Upload to mobile node 2
 *  Author      : Edgar Gomez, Tim Coulter
 *  Date        : 2 April 2017
 *  Version     : 1.0
 */
 
/*** Make sure to incoroporate the "beginRide" function - See the RSU ***/

//********** Library Includes **********//

#include <EasyTransfer.h>
#include <TinyGPS++.h>
#include <SD.h>

//********** Object Instantiation **********//

EasyTransfer ETin, ETout; 
TinyGPSPlus gps;
File dataFile;

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int nodeID = 4;
  double latErr;
  double lngErr;
};

struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int nodeID;
  double latErr;
  double lngErr;
};

//give a name to the group of data
RECEIVE_DATA_STRUCTURE rxdata;
SEND_DATA_STRUCTURE txdata;

//********** Variable Declaration **********//

bool RSU = false;
bool mobileNode1 = false;
bool mobileNode2 = false;
bool mobileNode3 = false;
       
int CS_PIN = 53;
int F = 0;
int i = 0;
int n_ii = 4;
int gpsPin = 19;
double gpsLat, gpsLng;
double x_prevLat, x_prevLng;
double x_newLat, x_newLng;
double neighLatError[4], neighLngError[4];
double myLatError,myLngError;

void setup(){
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(1000);
  //start the library, pass in the data details and the name of the serial port.
  ETin.begin(details(rxdata), &Serial);
  ETout.begin(details(txdata), &Serial);
  initializeSD();
}

void loop(){
  
  while (Serial1.available () > 0) {
    if (gps.encode(Serial1.read() )) {
      x_prevLat = gps.location.lat();
      x_prevLng = gps.location.lng();
      }
    }

    if (!RSU || !mobileNode1 || !mobileNode2 || !mobileNode3){
      if(ETin.receiveData()){
        switch(rxdata.nodeID){
          case 1:
            mobileNode1 = true;
            neighLatError[0] = rxdata.latErr; 
            neighLngError[0]= rxdata.lngErr; 
            break;
          case 2:
            mobileNode2 = true;
            neighLatError[1] = rxdata.latErr; 
            neighLngError[1] = rxdata.lngErr; 
            break;
          case 3:
            mobileNode3 = true;
            neighLatError[2] = rxdata.latErr; 
            neighLngError[2] = rxdata.lngErr; 
            break;
          case 5:
            RSU = true;
            neighLatError[3] = rxdata.latErr; 
            neighLngError[3] = rxdata.lngErr; 
            break;
            }
        
      }
    }

    if(RSU && mobileNode1 && mobileNode2 && mobileNode3){
      x_newLat = positionEstimateUpdate(x_prevLat, neighLatError, n_ii, F, "LAT");
       // Serial.print("myLatError "); Serial.println(myLatError,6);
       // Serial.print("my new Lat: "); Serial.println(x_newLat,6);

      x_newLng = positionEstimateUpdate(x_prevLng, neighLngError, n_ii, F,"");
        //Serial.print("myLngError "); Serial.println(myLngError,6);
        //Serial.print("my new Lng: "); Serial.println(x_newLng,6);
      txdata.latErr = myLatError;
      txdata.lngErr = myLngError;
      
      dataFile = SD.open("test.txt", FILE_WRITE);
      dataFile.print(x_newLat,6);
      dataFile.print(",");
      dataFile.println(x_newLng,6);
      dataFile.close(); // I had to add this or else the data would not get wrote to the SD card. It would create the file but not write anything. Once I closed the file it started working properly

      RSU = false;
      mobileNode1 = false;
      mobileNode2 = false;
      mobileNode3= false;
      }
 
    ETout.sendData();
    delay(100);
}


/**
 * Compare function to be used by quicksort to arrange elements
 * in ascending order.
 */
int qsortCompare(const void * arg1, const void * arg2) {
  double * a = (double *)arg1;  // cast to pointers to doubles
  double * b = (double *)arg2;

  // a less than b? 
  if (*a < *b)
    return -1;

  // a greater than b?
  if (*a > *b)
    return 1;

  // must be equal
  return 0;
}

double positionEstimateUpdate(double x_prev, double* diff_error_set, int n_ii, int F, String type) {

  // Initialize Local Variables
  int uprcount, lwrcount;

  // Sort error_set by Relative Bias and Estimated Relative Bias 
  qsort(&diff_error_set, n_ii, sizeof(double), qsortCompare);
  //Serial.print("diff err: "); Serial.println(diff_error_set[0],6);
  double *srtd_neigh = diff_error_set;
  double x_new;
  
  //If Number of neighbors is greater than Redundency Parameter
  if (n_ii>F) {

    // Remove values smaller than 0 in the F smallest values
    int lwrcount = 0;
    while (lwrcount<F && srtd_neigh[lwrcount]<0)
      lwrcount = lwrcount + 1;

    // Remove values larger than 0 in the F largest values
    int uprcount = n_ii;
    while (uprcount > n_ii - F && srtd_neigh[uprcount] > 0) {
      uprcount = uprcount - 1;
    }

    // Dynamically Allocate Memory for kept_neigh Array
    int kept_neigh_size = uprcount - lwrcount;
  double* kept_neigh = 0;
    kept_neigh = new double[kept_neigh_size];
    //Serial.print("kept Neigh Size: "); Serial.println(kept_neigh_size);
    //Serial.print("srtdneigh: "); Serial.println(srtd_neigh[0],6);
    //Serial.print("lwrcount: "); Serial.println(lwrcount,6);




    // Remove up to 2F lowest and 2F highest gps error estimates 
    for (int i = 0; i < kept_neigh_size; i++)
      kept_neigh[i] = srtd_neigh[i + lwrcount];
      //Serial.print("kept_neight[i] "); Serial.println(kept_neigh[0],6);
      
    
    // Calculate number of kept neighbors
    int num_kept = uprcount - lwrcount;
    //Serial.print("num_kept "); Serial.println(num_kept,6);

    // Calculate Weight to Use in Update (m_k)
    double m_k = 1.0 / (num_kept + 1);
    //Serial.print("m_k "); Serial.println(m_k,6);

    // Calculate Sum of Pseudo Errors 
    double sumPseudoErrors = 0;
    for (int i = 0; i<num_kept; i++)
      sumPseudoErrors += kept_neigh[i];
      //Serial.print("sumPseudoErrors "); Serial.println(sumPseudoErrors,6);
   
    if (type == "LAT") {
     
    // Calculate Leaky-Average of GPS Errors
    myLatError = m_k*sumPseudoErrors;    
    // Calculate position estimate (x_new)
    x_new = x_prev - myLatError;
    }
   
   else {
     myLngError = m_k*sumPseudoErrors;  
     x_new = x_prev - myLngError;
   }
  }

  else
    x_new = x_prev;
    
  return x_new;
}


void initializeSD()
{
  pinMode(10, OUTPUT); // Must declare 10 an output and reserve it to keep SD card happy
  SD.begin(CS_PIN);    // Initialize the SD card reader

 // Make sure the name of the file is small enough 
  if (SD.exists("test.txt")) { // Delete old data files to start fresh
      SD.remove("test.txt");
  }
}
