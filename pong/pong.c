#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <msp430.h>

#define LED BIT6		/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

// states:
// vertical ball direction
// horizontal ball direction

// top paddle position
// bottom paddle position

// paused

int paddle_dims[] = {30, 5};

// horiz, vert
int t_paddle_pos[] = {0, 10};
int b_paddle_pos[] = {0, 145};

void drawRect(int horiz, int vert, int shoriz, int svert, int color) {
  for (int i = horiz; i < horiz + shoriz; i++) {
    for (int j = vert; j < vert + svert; j++) {
      drawPixel(i, j, color);
    }
  }
}

void moveRect(
  int old_horiz, int old_vert, int old_shoriz, int old_svert, 
  int bg_color, int offset_horiz, int offset_vert, int color
) {
  drawRect(old_horiz, old_vert, old_shoriz, old_svert, bg_color);
  drawRect(old_horiz + offset_horiz, old_vert + offset_vert, old_shoriz, old_svert, color);
}

void main() {
  configureClocks();
  lcd_init();
  switch_init();
  enableWDTInterrupts(); // enable WDT
  or_sr(0x8); // enable interrupts
  
  // render -----------------
  int bg_clr = COLOR_BLACK;
  int obj_clr = COLOR_WHITE;
  clearScreen(bg_clr);

  // two paddles (for now)
  // drawRect(49, 10, 30, 5, COLOR_WHITE);
  // drawRect(49, 145, 30, 5, COLOR_WHITE);
  
  drawRect(t_paddle_pos[0], t_paddle_pos[1], paddle_dims[0], paddle_dims[1], obj_clr);  
  drawRect(b_paddle_pos[0], b_paddle_pos[1], paddle_dims[0], paddle_dims[1], obj_clr);  

  // top paddle
  for (int i = 0; i < 200; i++) {
    moveRect(
      t_paddle_pos[0], t_paddle_pos[1], paddle_dims[0], paddle_dims[1],
      bg_clr, 1, 0, obj_clr
    );
    t_paddle_pos[0] += 1;
  }
  // bottom paddle  
  for (int i = 0; i < 200; i++) {
    moveRect(
      b_paddle_pos[0], b_paddle_pos[1], paddle_dims[0], paddle_dims[1],
      bg_clr, 1, 0, obj_clr
    );
    b_paddle_pos[0] += 1;
  }

  // QUICK AND DIRTY: most efficient paddle moving
  // for (int i = 0; i < 20000; i++) {
  //   if (i % 100 != 0) continue;
  //   int j = i / 100;
  //   drawRect(b_paddle_horiz + j, b_paddle_vert, 1, paddle_svert, bg_clr);
  //   drawRect(j, b_paddle_vert, 1, paddle_svert, obj_clr);
  // }
  
  // play 25 cycles of sound while touching sidewalls, paddles

  // play 25 cycles of bad sound if anywhere beyond paddle
}

void switch_interrupt_handler() {
  
}

// watchdog timer
void __interrupt_vec(WDT_VECTOR) WDT() {
  // if caused by S1 - S4, handle with paddle-movers
  if (P2IFG & SWITCHES) {
    P2IFG &= ~SWITCHES; // clear sw interrupts
    switch_interrupt_handler();
  }

}