#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>

#define LED BIT6 /* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

// define side switch
#define SW0 BIT3

static char sw0_update_interrupt_sense() {
  char p1val = P1IN;
  /* update switch interrupt to detect changes from current buttons */
  P1IES |= (p1val & SW0);  /* if switch up, sense down */
  P1IES &= (p1val | ~SW0); /* if switch down, sense up */
  return p1val;
}

static char switch_update_interrupt_sense() {
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);  /* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES); /* if switch down, sense up */
  return p2val;
}

void 
sw0_init()			/* setup switch */
{  
  P1REN |= SW0;		/* enables resistors for SW0 */
  P1IE |= SW0;		/* enable interrupts from SW0 */
  P1OUT |= SW0;		/* pull-ups for SW0 */
  P1DIR &= ~SW0;		/* set SW0' bits for input */
  sw0_update_interrupt_sense();
}

void switch_init() /* setup switch */
{
  P2REN |= SWITCHES;  /* enables resistors for switches */
  P2IE |= SWITCHES;   /* enable interrupts from switches */
  P2OUT |= SWITCHES;  /* pull-ups for switches */
  P2DIR &= ~SWITCHES; /* set switches' bits for input */
  switch_update_interrupt_sense();
}

// states:
// vertical ball direction
// horizontal ball direction

// top paddle position
// bottom paddle position

// paused

// display resolution: 128 x 160
int display_dims[] = {128, 160};

// asset dimensions
int paddle_dims[] = {30, 5};
int ball_dims[] = {4, 4};

// horiz, vert
int t_paddle_pos[] = {98, 10};
int b_paddle_pos[] = {0, 145};
int ball_pos[] = {20, 20};

//// states
// switches
int switches = 0;

// asset directions (e.g. for paddles, -1 for left, 1 for right)
int t_paddle_dir[] = {-2, 0}; 
int b_paddle_dir[] = {2, 0};
int ball_dir[] = {1, 1};

void drawRect(int pos[], int dims[], int color) {
  for (int i = pos[0]; i < pos[0] + dims[0]; i++) {
    for (int j = pos[1]; j < pos[1] + dims[1]; j++) {
      drawPixel(i, j, color);
    }
  }
}

void moveRect(int pos[], int dims[], int dir[], int bg_color, int color) {
  drawRect(pos, dims, bg_color);
  pos[0] += dir[0];
  pos[1] += dir[1];
  drawRect(pos, dims, color);
}

int rangesOverlap(int r1[], int r2[]) {
  // be sure that the lower number is first
  return (r1[0] <= r2[1] && r1[0] >= r2[0]) 
    || (r2[0] <= r1[1] && r2[0] >= r1[0]);
}

int boxesCollide(int b1pos[], int b1dims[], int b2pos[], int b2dims[]) {

  // check horizontal overlap
  int b1_bounds_horiz[] = {b1pos[0], b1pos[0] + b1dims[0]};
  int b2_bounds_horiz[] = {b2pos[0], b2pos[0] + b2dims[0]};
  int horiz_overlap = rangesOverlap(b1_bounds_horiz, b2_bounds_horiz);
  
  // check vertical overlap
  int b1_bounds_vert[] = {b1pos[1], b1pos[1] + b1dims[1]};
  int b2_bounds_vert[] = {b2pos[1], b2pos[1] + b2dims[1]};
  int vert_overlap = rangesOverlap(b1_bounds_vert, b2_bounds_vert);

  return horiz_overlap && vert_overlap;
}

void main() {
  configureClocks();
  lcd_init();
  switch_init();
  sw0_init();
  enableWDTInterrupts();  // enable WDT
  or_sr(0x8);             // enable interrupts

  //// render -----------------
  int bg_clr = COLOR_BLACK;
  int obj_clr = COLOR_WHITE;
  clearScreen(bg_clr);

  // draw paddles
  drawRect(t_paddle_pos, paddle_dims, obj_clr);
  drawRect(b_paddle_pos, paddle_dims, obj_clr);

  // draw ball
  drawRect(ball_pos, ball_dims, obj_clr);

  // continuous game loop
  while (1) {
    // update top paddle
    moveRect(t_paddle_pos, paddle_dims, t_paddle_dir, bg_clr, obj_clr);

    // update bottom paddle
    moveRect(b_paddle_pos, paddle_dims, b_paddle_dir, bg_clr, obj_clr);

    // update ball 
    moveRect(ball_pos, ball_dims, ball_dir, bg_clr, obj_clr);

    // is ball hitting a wall
    if ((ball_pos[0] <= 0) || (ball_pos[0] + ball_dims[0] >= display_dims[0])) {
      ball_dir[0] = -ball_dir[0];
      // TODO: beep
    }
    
    if ( // is ball hitting bottom paddle?
      boxesCollide(b_paddle_pos, paddle_dims, ball_pos, ball_dims)
    ) {
      ball_dir[1] = -1;
    }

    if ( // is ball hitting top paddle?
      boxesCollide(t_paddle_pos, paddle_dims, ball_pos, ball_dims)
    ) {
      ball_dir[1] = 1;
    }

    // did ball exit the field?
    if (ball_pos[1] <= 0 || ball_pos[1] + ball_dims[1] >= display_dims[1]) {
      clearScreen(COLOR_RED);
      break;
    }

    // did top paddle hit the edge?
    if (t_paddle_pos[0] <= 0 || t_paddle_pos[0] + paddle_dims[0] >= display_dims[0]) {
      t_paddle_dir[0] = 0;
    }

    // did bottom paddle git the edge?
    if (b_paddle_pos[0] <= 0 || b_paddle_pos[0] + paddle_dims[0] >= display_dims[0]) {
      b_paddle_dir[0] = 0;
    }
  }
}

void sw0_interrupt_handler() {
  // save toggled switches and flip sensitivity
  char p1val = sw0_update_interrupt_sense();
  switches = ~p1val & SW0;

  // sw0 will handle sw1 logic
  if (switches & SW0) t_paddle_dir[0] = -2;
}

void switch_interrupt_handler() {
  // save toggled switches and flip sensitivity
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;

  // // s1 --> set top dir to -1 if 0, else 0
  // if (switches & SW1) t_paddle_dir[0] = -2;

  // // s2 --> set top dir to 1 if 0, else 0
  // if (switches & SW2) t_paddle_dir[0] = 2;

  // sw1 is now sw2 because sw2 is problematic
  if (switches & SW1) t_paddle_dir[0] = 2;

  // s3 --> set bottom dir to -1 if 0, else 0
  if (switches & SW3) b_paddle_dir[0] = -2;

  // s4 --> set bottom dir to 1 if 0, else 0
  if (switches & SW4) b_paddle_dir[0] = 2;
}

void __interrupt_vec(PORT1_VECTOR) port_1() {
  if (P1IFG & SW0) {
    P1IFG &= ~SW0;
    sw0_interrupt_handler();
  }
}

// for front buttons
void __interrupt_vec(PORT2_VECTOR) Port_2() {
  if (P2IFG & SWITCHES) {
    // call the switch handler
    P2IFG &= ~SWITCHES;
    switch_interrupt_handler();
  }
}

// watchdog timer
void __interrupt_vec(WDT_VECTOR) WDT() {
}
