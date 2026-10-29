#if defined(TEENSYDUINO)
#include <usb_names.h>

#define MANUFACTURER_NAME {'B','l','a','s','t',' ','B','e','a','t','s'}
#define MANUFACTURER_NAME_LEN 11

#define PRODUCT_NAME {'B','L','A','S','T',' ','B','E','A','T','S',' ','M','I','D','I'}
#define PRODUCT_NAME_LEN 16

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  2 + MANUFACTURER_NAME_LEN * 2,
  3,
  MANUFACTURER_NAME
};

struct usb_string_descriptor_struct usb_string_product_name = {
  2 + PRODUCT_NAME_LEN * 2,
  3,
  PRODUCT_NAME
};
#endif
