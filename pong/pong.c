#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include "buzzer.h"
#include "lcddraw.h"
#include "lcdutils.h"

#define LED BIT6 /* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

// define side switch
#define SW0 BIT3

// constants
const int BG_CLR = COLOR_BLACK;
const int OBJ_CLR = COLOR_WHITE;

const int BNC_TONE = 512;
const int LOSE_TONE = 2048;
const int QUIET_TONE = 8;
const int BUZZ_LEN = 10;

const int BALL_POS_INIT[] = {30, 10};
const int BALL_DIR_INIT[] = {1, 1};

int DISPLAY_DIMS[] = {128, 160};

// buzzer attrs
int buzz_remaining = 0;
int buzz_tone = BNC_TONE;

// scores
char t_score = 0;
char b_score = 0;

// asset dimensions
int paddle_dims[] = {30, 5};
int ball_dims[] = {3, 3}; 

// horiz, vert, old horiz, old vert
// old vals used for erasing previous pixels.
int t_paddle_pos[] = {98, 10, 98, 10};
int b_paddle_pos[] = {0, 145, 0, 145};
int ball_pos[] = {
  BALL_POS_INIT[0], BALL_POS_INIT[1], BALL_POS_INIT[0], BALL_POS_INIT[1]
};

// do we need to render anything? 
char redrawScreen = 0;
int switches = 0;

// asset directions (e.g. for paddles, -1 for left, 1 for right)
int t_paddle_dir[] = {-2, 0}; 
int b_paddle_dir[] = {2, 0};
int ball_dir[] = {1, 1};

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
  // snapshot current rectangle (to avoid changes while drawing)
  int curr_rect_pos[] = {pos[0], pos[1]};

  // get old rectangle pos
  int old_rect_pos[] = {pos[2], pos[3]};

  // clear old rectangle
  drawRect(old_rect_pos, dims, BG_CLR);

  // new reclangle pos are implicit since first and second index don't change
  // draw new rectangle
  drawRect(curr_rect_pos, dims, OBJ_CLR);

  // save current reclangle as old position
  pos[2] = curr_rect_pos[0];
  pos[3] = curr_rect_pos[1];
}

// clear only non-overlapping old pixels, draw non-overlapping new ones
// only cares about x. it's hard (and unnecessary) to do both axes
void moveRectDiffX(int pos[], int dims[]) {
  // snapshot old rectangle (to avoid change while drawing)
  int curr_rect_pos[] = {pos[0], pos[1]};

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

  // cover freed pixels
  drawRect(freed_rect_pos, freed_rect_dims, BG_CLR);

  // draw new pixels
  drawRect(curr_rect_pos, dims, OBJ_CLR);  

  // save current rectangle pos as old state
  pos[2] = curr_rect_pos[0];
  pos[3] = curr_rect_pos[1];
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
  // update top score
  drawChar5x7(DISPLAY_DIMS[0] / 2 - 2, 0, 48 + t_score, OBJ_CLR, BG_CLR);

  // update bottom score
  drawChar5x7(
    DISPLAY_DIMS[0] / 2 - 2, DISPLAY_DIMS[1] - 7, 48 + b_score, OBJ_CLR, BG_CLR
  );

  // update physics
  moveRect(ball_pos, ball_dims);
  moveRectDiffX(t_paddle_pos, paddle_dims);
  moveRectDiffX(b_paddle_pos, paddle_dims);
}

void wdt_c_handler() {
  // don't do anything if game over
  if (t_score > 5 || b_score > 5) {
    return;
  }

  static int secCount = 1;
  secCount ++;

  // set buzzer if necessary
  if (buzz_remaining > 0) {
    buzz_remaining--;
    buzzer_set_period(buzz_tone);
  } else if (buzz_remaining == 0) {
    buzzer_set_period(QUIET_TONE);
  }

  // update physical params
  if (secCount % 5 == 0) {
    secCount = 1;
    
    // now we gotta render
    redrawScreen = 1;

    // update positions
    ball_pos[0] += ball_dir[0];
    ball_pos[1] += ball_dir[1];

    t_paddle_pos[0] += t_paddle_dir[0];
    t_paddle_pos[1] += t_paddle_dir[1];

    b_paddle_pos[0] += b_paddle_dir[0];
    b_paddle_pos[1] += b_paddle_dir[1];

    // is ball hitting a wall
    if ((ball_pos[0] <= 0) || (ball_pos[0] + ball_dims[0] >= DISPLAY_DIMS[0])) {
      ball_dir[0] = -ball_dir[0];
      buzz_remaining = BUZZ_LEN;
      buzz_tone = BNC_TONE;
    }
    
    if ( // is ball hitting bottom paddle?
      boxesCollide(b_paddle_pos, paddle_dims, ball_pos, ball_dims)
    ) {
      ball_dir[1] = -1;
      buzz_remaining = BUZZ_LEN;
      buzz_tone = BNC_TONE;
    }

    if ( // is ball hitting top paddle?
      boxesCollide(t_paddle_pos, paddle_dims, ball_pos, ball_dims)
    ) {
      ball_dir[1] = 1;
      buzz_remaining = BUZZ_LEN;
      buzz_tone = BNC_TONE;
    }

    // did ball exit field?
    if (ball_pos[1] <= 0 || ball_pos[1] + ball_dims[1] >= DISPLAY_DIMS[1]) {
      if (ball_pos[1] <= 0) {
        b_score++;
      } else {
        t_score++;
      }
      buzz_remaining = BUZZ_LEN;
      buzz_tone = LOSE_TONE;
      // reposition ball
      ball_pos[0] = BALL_POS_INIT[0];
      ball_pos[1] = BALL_POS_INIT[1];
      // reset ball direction
      ball_dir[0] = BALL_DIR_INIT[0];
      ball_dir[1] = BALL_DIR_INIT[1];
    }

    // did top paddle hit the edge?
    if (t_paddle_pos[0] <= 0 || t_paddle_pos[0] + paddle_dims[0] >= DISPLAY_DIMS[0]) {
      t_paddle_dir[0] = 0;
    }

    // did bottom paddle git the edge?
    if (b_paddle_pos[0] <= 0 || b_paddle_pos[0] + paddle_dims[0] >= DISPLAY_DIMS[0]) {
      b_paddle_dir[0] = 0;
    }
  }
}

void main() {
  
  P1DIR |= LED;		/**< Green led on when CPU on */
  P1OUT |= LED;
  configureClocks();
  buzzer_init();
  lcd_init();
  switch_init();
  sw0_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  clearScreen(BG_CLR);

  // draw paddles
  drawRect(t_paddle_pos, paddle_dims, OBJ_CLR);
  drawRect(b_paddle_pos, paddle_dims, OBJ_CLR);

  // draw ball
  drawRect(ball_pos, ball_dims, OBJ_CLR);

  while (t_score < 5 && b_score < 5) {
    if (redrawScreen) {
      redrawScreen = 0;

      update_shape();
    }
    P1OUT &= ~LED;	/* led off */
    or_sr(0x10);	/**< CPU OFF */
    P1OUT |= LED;	/* led on */ 
  }

  // game over
  clearScreen(BG_CLR);
  if (t_score > b_score) {
    drawString5x7(0, 0, "Top player won!", OBJ_CLR, BG_CLR);
  } else {
    drawString5x7(0, 0, "Bottom player won!", OBJ_CLR, BG_CLR);
  }
  P1OUT &= ~LED;	/* led off */
  or_sr(0x10);	/**< CPU OFF */
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

// for side button
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
