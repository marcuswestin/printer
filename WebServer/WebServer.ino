#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>

#include "HTTPServer.h"

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0A, 0x00, 0x54 };
IPAddress ip(10,0,0,10);


static char doctype[] = "<!doctype html>";
static char form[] = "<form method=POST action=/txt>"
  "<textarea name=s></textarea>"
  "<input type=submit value=Print!>"
"</form>";

EthernetServer server(80);
SoftwareSerial printer(
  2, // RX - green wire
  3  // TX - yellow wire
);

void setup() {
  Ethernet.begin(mac, ip);
  server.begin();
  printer.begin(19200);
  printer.write(27); printer.write(64); // reset
  printer.write(27); printer.write(61); printer.write(1); // online

  printer.write(27); printer.write(55);   // Esc 7 (print settings)
  printer.write(20);       // Heating dots (20=balance of darkness vs no jams)
  printer.write(255);      // Library default = 255 (max)
  printer.write(250);      // Heat interval (500 uS = slower, but darker)

#define printDensity   14 // 120% (? can go higher, text is darker but fuzzy)
#define printBreakTime  4 // 500 uS

  printer.write(18); printer.write(35); // DC2 # (print density)
  printer.write((printBreakTime << 5) | printDensity);
}


typedef struct {
  uint32_t size;
  uint16_t reserved1, reserved2;
  uint32_t offset;
} __attribute__ ((packed)) bmp_header;

typedef struct {
  uint32_t size;
  int32_t width, height;
  uint16_t planes;
  uint16_t bits;
  uint32_t compression;
  uint32_t imagesize;
  int32_t xresolution, yresolution;
  uint32_t ncolors;
  uint32_t importantcolours;
} __attribute__ ((packed)) bmp_infoheader;

int print_bmp(Stream *bmp, Stream *printer) {
  if (bmp->read() != 'B' || bmp->read() != 'M') { return -1; }

  bmp_header header;
  bmp_infoheader infoheader;

  bmp->readBytes((char *)&header, sizeof(header));
  bmp->readBytes((char *)&infoheader, sizeof(infoheader));

  if (infoheader.compression != 0 || infoheader.bits != 1) { return -2; }

  // skip over the colour index
  for (int i = 0; i < 4*infoheader.ncolors; i++) { bmp->read(); }

  if (infoheader.width != 384) { return -3; }
  for (int i = 0; i < infoheader.height; i += 255) {
    int height = min(255, infoheader.height - i);
    printer->write(18); printer->write(42);
    printer->write(height); printer->write(infoheader.width/8);
    for (int j = 0; j < (height * infoheader.width)/8; j++) {
      printer->write(~bmp->read());
    }
  }

  return 0;
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    HTTPRequest request(&client);

    if (request.error_code) {
      // TODO(paulsowden) why does print sometimes and sometimes not end with a trailing space
      client.print("HTTP/1.0 "); client.print(request.error_code); client.println(" Fiddlesticks");
      client.println("Content-type: text/plain");
      client.println();
      client.println("Fiddlesticks!");
      client.stop();
      return;
    }

    client.println("HTTP/1.0 200 OK");
    client.println("Content-type: text/html");
    client.println();
    client.println(doctype);

    if (request.method == HTTP_GET && strcmp("/", request.url)) {
      client.println(form);
    } else if (request.method == HTTP_POST && strcmp("/txt", request.url)) {
      // Print plain text input
      client.println("Printing:");
      client.println("<pre>");
      client.read(); client.read(); // pop off the "s="
      while (client.available()) {
        char c = client.read();
        client.print(c);
        printer.print(c);
      }
      client.println("</pre>");
      printer.print('\n');
      client.println("<a href=/>Again!</a>");
    } else if (request.method == HTTP_POST && strcmp("/img", request.url)) {
      HTTPEntity entity(&request);
      print_bmp(&entity, &printer);
    }

    client.stop();
  }
}

