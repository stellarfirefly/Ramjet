#include <SD.h>
#include <Wire.h>
#include <LSM6.h>
#include <LIS3MDL.h>
#include <LPS.h>

LSM6 lsm6AccGyro;
LIS3MDL lis3Magneto;
LPS lpsBaroAlt;

const int chipSelectSD = 4;   // pin select for SD card
const String fileAccGyro = "ACCGYRO.TXT";
const String fileMagneto = "MAGNETO.TXT";
const String fileBaroAlt = "BAROALT.TXT";
const int buffSize = 80;    // size of character buffers

int runLoopCounter = 0;   // loops in current run
    // loop delay in milliseconds, set to 1000 for testing, 250 for actual run
const int runLoopDelay = 250;

char report[ buffSize ];    // serial status output buffer

void setup() {
  Serial.begin( 9600 );
  Wire.begin();
  delay( 10000 );   // 10 sec delay to allow for communications initialization time

  printStatus( "Initializing SD card..." );
  if( !SD.begin( chipSelectSD )){
    errorCritical( "Failed to initialize SD card." );
  }
  
  printStatus( "Initializing accelerometer/gyroscope..." );
  if( !lsm6AccGyro.init() ){
    errorCritical( "Failed to detect and initialize accelerometer/gyroscope." );
  }
  lsm6AccGyro.enableDefault();
  writeSDFile( fileAccGyro, "; data begin - ord:time:A:ax,ay,az:G:gx,gy,gz" );

  printStatus( "Initializing magnetometer..." );
  if( !lis3Magneto.init() ){
    errorCritical( "Failed to detect and initialize magnetometer." );
  }
  lis3Magneto.enableDefault();
  writeSDFile( fileMagneto, "; data begin - ord:time:M:mx,my,mz" );

  printStatus( "Initializing barometer/altimeter/thermometer..." );
  if( !lpsBaroAlt.init() ){
    errorCritical( "Failed to detect and initialize barometer/altimeter/thermometer." );
  }
  lpsBaroAlt.enableDefault();
  writeSDFile( fileBaroAlt, "; data begin - ord:time:P:pf:A:af:T:tf" );
}

void loop() {
  char strLoopCount[ buffSize ];

  // we don't have an RTC, so we'll just enumerate with a loop counter
  // and use it to prefix each line of the reports
  snprintf( strLoopCount, sizeof(strLoopCount), "%06d", runLoopCounter );
  
  // process accelerometer/gyroscope data from LSM6
  lsm6AccGyro.read();
  snprintf( report, sizeof(report), "%s:%ld:A:%d,%d,%d:G:%d,%d,%d", strLoopCount, millis(),
    lsm6AccGyro.a.x, lsm6AccGyro.a.y, lsm6AccGyro.a.z,
    lsm6AccGyro.g.x, lsm6AccGyro.g.y, lsm6AccGyro.g.z );
  Serial.println( report );
  writeSDFile( fileAccGyro, report );
  
  // process magnetometer data from LIS3MDL
  lis3Magneto.read();
  snprintf( report, sizeof(report), "%s:%ld:M:%d,%d,%d", strLoopCount, millis(),
    lis3Magneto.m.x, lis3Magneto.m.y, lis3Magneto.m.z );
  Serial.println( report );
  writeSDFile( fileMagneto, report );

  // process barometer/altimeter data from LPS
  float baPressure = lpsBaroAlt.readPressureMillibars();
  float baAltitude = lpsBaroAlt.pressureToAltitudeMeters( baPressure );
  float baTemperature = lpsBaroAlt.readTemperatureC();
  snprintf( report, sizeof(report), "%s:%ld:P:%d.%04d:A:%d.%04d:T:%d.%04d", strLoopCount, millis(),
    int(baPressure), int(10000*(baPressure-int(baPressure))),
    int(baAltitude), int(10000*(baAltitude-int(baAltitude))),
    int(baTemperature), int(10000*(baTemperature-int(baTemperature))) );
  Serial.println( report );
  writeSDFile( fileBaroAlt, report );

  delay( runLoopDelay );
  ++runLoopCounter;
}

/* utility functions */

//---- output line to file on SD card
void writeSDFile( String filename, String message ){
  char buffer[80];
  
  File dataFile = SD.open( filename, FILE_WRITE );
  if( dataFile ){
    dataFile.println( message );
    dataFile.close();
  }else{
    filename.toCharArray( buffer, sizeof(buffer) );
    snprintf( report, sizeof(report), "Unable to write to file: %s", buffer );
    printStatus( report );
  }
}

//---- output critical error message to serial port then halt
void errorCritical( String msg ){
  Serial.print( "ERROR: Critical: " );
  Serial.println( msg );
  while(1);
}

//---- output simple status message to serial port
void printStatus( String msg ){
  Serial.print( "STATUS: " );
  Serial.println( msg );
}
