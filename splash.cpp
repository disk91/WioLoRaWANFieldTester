#include <Arduino.h>
#include "config.h"
#include "helium_logo.h"
#include "ttn_logo.h"
#include "ui.h"

void draw_splash_helium(int xOffset, int yOffset, int density) {

  char * logo = helium_data;
  for ( int y = yOffset ; y < yOffset+helium_height ; y++ ) {
    for ( int x  = xOffset ; x < xOffset+helium_width ; x++) {
        uint8_t pixel[3];
        HELIUM_PIXEL(logo,pixel);
        uint16_t r = ((pixel[0] >> 3) * density) / 100;
        uint16_t g = ((pixel[1] >> 2) * density) / 100;
        uint16_t b = ((pixel[2] >> 3) * density) / 100;
        uint16_t color = ( ( r << 11 ) & 0xF800 ) | ( ( g << 5 ) & 0x07E0 ) | ( b & 0x001F ); 
        if ( pixel[2] > 2*pixel[0] ) { 
          tft.drawPixel(x,y,color);
        }
    }
  }
  
}

void draw_splash_ttn(int xOffset, int yOffset, int density) {

  char * logo = ttn_data;
  for ( int y = yOffset ; y < yOffset+ttn_height ; y++ ) {
    for ( int x  = xOffset ; x < xOffset+ttn_width ; x++) {
        uint8_t pixel[3];
        uint16_t r,g,b;
        HELIUM_PIXEL(logo,pixel);
        if ( pixel[0]+pixel[1]+pixel[2] < 50 ) {
          r = ((0xFF >> 3) * density) / 100;
          g = ((0xFF >> 2) * density) / 100;
          b = ((0xFF >> 3) * density) / 100;         
        } else {
          r = ((pixel[0] >> 3) * density) / 100;
          g = ((pixel[1] >> 2) * density) / 100;
          b = ((pixel[2] >> 3) * density) / 100;
        }
        uint16_t color = ( ( r << 11 ) & 0xF800 ) | ( ( g << 5 ) & 0x07E0 ) | ( b & 0x001F ); 
        if ( pixel[2] > 2*pixel[0] || ( pixel[0]+pixel[1]+pixel[2] < 50 ) ) { 
          tft.drawPixel(x,y,color);
        }
    }
  }
  
}
