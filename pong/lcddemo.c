/** \file lcddemo.c
 *  \brief A simple demo that draws a string and square
 */

#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"

f1() {
  for (int i = 0; i < 128; i++)
    drawPixel(i, i, COLOR_PINK);
}

f2() {
  for (int i = 0; i < 30; i++) {
    drawPixel(i, 30-i, COLOR_PINK);
  }
}

// centered_x

ftriangle() {
  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < i; j++) {
      drawPixel(j, i, COLOR_BEIGE);
    }
  }
}

fx() {
  int color = 0;
  for (int i = 0; i < 128; i++) {
    for (int j = 0; j < i; j++) {
      color = i % 20 < 10 ? COLOR_BLUE : COLOR_RED;
      drawPixel(i,i, color);
    }
  }
}

one_arrow(int color, int offset_i, int offset_j) {
  for (int i = 0; i < 20; i++) {
    for (int j = 20; j >= 0; j--) {
      if (j > 20 - i) {
        // drawPixel(i + offset_i, j + offset_j, color);
        drawPixel(i + offset_i, j + offset_j, color);
      }
    }
  }
}

bunch_of_arrows(){
  for (int i = 0; i * 20 < 128; i++) {
    int color = COLOR_BLUE;
    if (i % 2 == 0)
      color = COLOR_RED;
    one_arrow(color, i * 20, i * 20);
  }
}

/** Initializes everything, clears the screen, draws "hello" and a square */
int
main()
{
  configureClocks();
  lcd_init();
  u_char width = screenWidth, height = screenHeight;

  clearScreen(COLOR_BLACK);

  // drawString5x7(20,20, "LMAO", COLOR_GREEN, COLOR_RED);

  // fillRectangle(30,30, 60, 60, COLOR_BLACK);

  // f1();

  // fx();

  bunch_of_arrows();

  // one_arrow(COLOR_AQUAMARINE, 0, 0);
  
}

