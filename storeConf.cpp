#include <FlashStorage.h>
#include "testeur.h"
#include "ui.h"

#define MAGIC 0xD154
#define VERSION 0x01

typedef struct {
  uint16_t  magic;
  uint8_t   version;
  int8_t    cPwr;     // Current Power
  uint8_t   cSf;      // Current SF
  uint8_t   cRetry;   // Current Number of retry
  uint8_t   selected_display;
} Config;


FlashStorage(my_flash_store, Config);

// Load configuration from flash
// return tru if loaded false if default must be set
bool readConfig() {

  Config c = my_flash_store.read();
  if ( c.magic == MAGIC && c.version == VERSION ) {
    state.cPwr = c.cPwr;
    state.cSf = c.cSf;
    state.cRetry = c.cRetry;
    ui.selected_display = c.selected_display;
  } else {
    return false;
  }
  return true;
  
}

void storeConfig() {
  Config c;
  c.magic = MAGIC;
  c.version = VERSION;
  c.cPwr = state.cPwr;
  c.cSf = state.cSf;
  c.cRetry = state.cRetry;
  c.selected_display = ui.selected_display;
  my_flash_store.write(c);
}
