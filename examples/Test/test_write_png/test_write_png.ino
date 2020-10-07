/*\
 *
 * PNG Write test using miniz
 * Loosely coded by tobozo for LovyanGFX
 * copyleft October 2020
 *
 *
 * !!!  This example REQUIRES Psram  !!!
 *
 *
\*/
#include <SD.h>
#include <LGFX_TFT_eSPI.h>
#include "lgfx/utility/miniz.h"
static LGFX tft;

const char* PNG_FILE = "/test.png";

void setup() {

  Serial.begin( 115200 );

  tft.begin();
  tft.setTextDatum( TC_DATUM );
  tft.setTextSize(2);
  tft.setTextColor( TFT_WHITE, TFT_BLACK );

  SD.begin(4);

  if( !psramInit() ) {
    log_n("ERROR: This feature needs psram and none was found, aborting");
    return;
  } else {
    log_w("PSRAM OK");
  }

  int captureWidth  = tft.width();
  int captureHeight = tft.height();

  // fill capture area with random circles
  for( int i=0;i<100;i++ ) {
    int x = random( captureWidth );
    int y = random( captureHeight );
    uint16_t color = random( 65536 );
    byte radius = random( captureWidth/6 );
    tft.fillCircle(x, y, radius, color );
  }
  // screen capture
  RGBColor *imagebuff = (RGBColor*)ps_malloc( (captureWidth*captureHeight*3)+1 );
  if( imagebuff == nullptr ) {
    log_n( "ERROR: Can't malloc image, aborting" );
    return;
  }
  tft.readRectRGB(0, 0, captureWidth, captureHeight, imagebuff );

  size_t png_data_size = 0;
  uint32_t time_start = millis();
  {
    void *pPNG_data = tdefl_write_image_to_png_file_in_memory_ex(imagebuff, captureWidth, captureHeight, 3, &png_data_size, 6, 0);
    if (!pPNG_data)
      log_e("tdefl_write_image_to_png_file_in_memory_ex() failed!\n");
    else
    {
      File outFile = SD.open(PNG_FILE, "wb");
      outFile.write((uint8_t *)pPNG_data, png_data_size);
      outFile.close();
      Serial.printf("Wrote %s. Total time %u ms\n", PNG_FILE, millis()-time_start);
    }

    free(pPNG_data);
  }
  free(imagebuff);

  // pre-fill display with red
  tft.fillScreen( TFT_BLACK );
  tft.drawString("Reading PNG form SD..", captureWidth/2, captureHeight/2);

  if( png_data_size >= 0 )  {
    // show a hex dump of the PNG file in Serial console
    hexDumpFile( SD, PNG_FILE );
    // if drawPngFile failed, the display stays red
    tft.drawPngFile( SD, PNG_FILE );
    Serial.println("Test Finished");
  } else {
    tft.fillScreen( TFT_RED );
    //tft.setCursor( tft.width()/2, tft.height()/2);
    tft.drawString("Rendering Failed :-(", captureWidth/2, captureHeight/2);
    Serial.println("Test failed ");
  }

  delay( 10000 );
  tft.setBrightness(0);

}

void loop() {

}


// show the contents of a given file as a hex dump
static void hexDumpFile( fs::FS &fs, const char* filename) {
  File pngOut = fs.open(filename);
  log_w("File size : %d", pngOut.size() );
  if( pngOut.size() > 0 ) {
    size_t output_size = 32;
    char* buff = new char[output_size];
    uint8_t bytes_read = pngOut.readBytes( buff, output_size );
    String bytesStr  = "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00";
    String binaryStr = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    String addrStr = "[0x0000 - 0x0000] ";
    char byteToStr[32];
    size_t totalBytes = 0;
    while( bytes_read > 0 ) {
      bytesStr = "";
      binaryStr = "";
      for( int i=0; i<bytes_read; i++ ) {
        sprintf( byteToStr, "%02X", buff[i] );
        bytesStr  += String( byteToStr ) + String(" ");
        if( isprint( buff[i] ) ) {
          binaryStr += String( buff[i] );
        } else {
          binaryStr += " ";
        }
      }
      sprintf( byteToStr, "[0x%04X - 0x%04X] ",  totalBytes, totalBytes+bytes_read);
      totalBytes += bytes_read;
      if( bytes_read < output_size ) {
        for( int i=0; i<output_size-bytes_read; i++ ) {
          bytesStr  += "00 ";
          binaryStr += " ";
        }
      }
      Serial.println( byteToStr + bytesStr + " " + binaryStr );
      bytes_read = pngOut.readBytes( buff, output_size );
    }
  }
  pngOut.close();
}


