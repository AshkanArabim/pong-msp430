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

// end game if it's over
char game_over = 0;

// display resolution: 128 x 160
int display_dims[] = {128, 160};

// asset dimensions
int paddle_dims[] = {30, 5};
int ball_dims[] = {3, 3}; 

// horiz, vert, old horiz, old vert
// old vals used for erasing previous pixels.
int t_paddle_pos[] = {98, 10, 98, 10};
int b_paddle_pos[] = {0, 145, 0, 145};
int ball_pos[] = {20, 20, 20, 20};

// do we need to render anything? 
char redrawScreen = 0;
int switches = 0;

// asset directions (e.g. for paddles, -1 for left, 1 for right)
int t_paddle_dir[] = {-2, 0}; 
int b_paddle_dir[] = {2, 0};
int ball_dir[] = {1, 1};

int bg_clr = COLOR_BLACK;
int obj_clr = COLOR_WHITE;

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

void drawRect(int pos[], int dims[], int color) {
  for (int i = pos[0]; i < pos[0] + dims[0]; i++) {
    for (int j = pos[1]; j < pos[1] + dims[1]; j++) {
      drawPixel(i, j, color);
    }
  }
}

void moveRect(int pos[], int dims[]) {
  // get old rectangle pos
  int old_rect_pos[] = {pos[2], pos[3]};
  // clear old rectangle
  drawRect(old_rect_pos, dims, bg_clr);

  // new reclangle pos are implicit since first and second index don't change
  // draw new rectangle
  drawRect(pos, dims, obj_clr);
}

// clear only non-overlapping old pixels, draw non-overlapping new ones
// only cares about x. it's hard (and unnecessary) to do both axes
void moveRectDiffX(int pos[], int dims[]) {
  int freed_rect_dims[] = {pos[0] - pos[2], dims[1]};
  char goingleft = 0;
  if (freed_rect_dims[0] < 0) {
    freed_rect_dims[0] = -freed_rect_dims[0];
    goingleft = 1;
  }
  
  // get starting position of difference rectangle
  int freed_rect_pos[] = {pos[2], pos[1]};
  if (goingleft) {
    freed_rect_pos[0] = pos[0] + dims[0];
  }

  // \\confirmed//
  // cover freed pixels
  drawRect(freed_rect_pos, freed_rect_dims, bg_clr);

  // draw new pixels
  drawRect(pos, dims, obj_clr);  
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

void update_shape() {
  moveRect(ball_pos, ball_dims);
  moveRectDiffX(t_paddle_pos, paddle_dims);
  moveRectDiffX(b_paddle_pos, paddle_dims);
}

void wdt_c_handler() {
  static int secCount = 1;

  secCount ++;

  // TODO: reduce frequency for ALL of this
  if (secCount % 5 == 0) {
    secCount = 1;
    
    // now we gotta render
    redrawScreen = 1;

    // update positions
    ball_pos[2] = ball_pos[0];
    ball_pos[3] = ball_pos[1];
    ball_pos[0] += ball_dir[0];
    ball_pos[1] += ball_dir[1];

    t_paddle_pos[2] = t_paddle_pos[0];
    t_paddle_pos[3] = t_paddle_pos[1];
    t_paddle_pos[0] += t_paddle_dir[0];
    t_paddle_pos[1] += t_paddle_dir[1];

    b_paddle_pos[2] = b_paddle_pos[0];
    b_paddle_pos[3] = b_paddle_pos[1];
    b_paddle_pos[0] += b_paddle_dir[0];
    b_paddle_pos[1] += b_paddle_dir[1];

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
      game_over = 1;
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

void main() {
  
  P1DIR |= LED;		/**< Green led on when CPU on */
  P1OUT |= LED;
  configureClocks();
  lcd_init();
  switch_init();
  sw0_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  clearScreen(bg_clr);

  // draw paddles
  drawRect(t_paddle_pos, paddle_dims, obj_clr);
  drawRect(b_paddle_pos, paddle_dims, obj_clr);

  // draw ball
  drawRect(ball_pos, ball_dims, obj_clr);

  while (!game_over) {
    if (redrawScreen) {
      redrawScreen = 0;

      // DEBUG: check if this even gets called --> yes.
      // clearScreen(COLOR_RED);
      // return;

      update_shape();
    }
    P1OUT &= ~LED;	/* led off */
    or_sr(0x10);	/**< CPU OFF */
    P1OUT |= LED;	/* led on */ 
  }

  // game over
  clearScreen(COLOR_RED);
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
