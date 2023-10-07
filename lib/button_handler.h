// keys
#define KEY_SHIFT 0
#define KEY_A 1
#define KEY_B 2
#define KEY_C 3
#define MODE_JUMP 0
#define MODE_MASH 1
#define MODE_SAMP 0
#define MODE_BANK 1

uint8_t key_held_num = 0;
bool key_held_on = false;
int32_t key_timer = 0;
uint8_t key_pressed[100];
uint8_t key_pressed_num = 0;
uint8_t key_total_pressed = 0;
int16_t key_on_buttons[BUTTONMATRIX_BUTTONS_MAX];
uint16_t key_num_presses;

// mode toggles
//   mode  ==0  ==1
bool mode_jump_mash = 0;
bool mode_samp_bank = 0;

// fx toggles
bool fx_toggle[16];  // 16 possible
#define FX_REVERSE 0

void key_do_jump(uint8_t beat) {
  if (beat >= 0 && beat < 16) {
    beat_current = (beat_current / 16) * 16 + beat;
    do_update_phase_from_beat_current();
    LEDS_clearAll(leds, LED_STEP_FACE);
    LEDS_set(leds, LED_STEP_FACE, beat_current % 16 + 4, 2);
  }
}

// uptate all the fx based on the fx_toggle
void do_update_fx(uint8_t fx_num) {
  bool on = fx_toggle[fx_num];
  switch (fx_num) {
    case FX_REVERSE:
      phase_forward = !on;
      break;
    default:
      break;
  }
}

void button_key_off_held(uint8_t key) {
  printf("off held %d\n", key);
  if (key == KEY_A) {
    // bank/sample toggler
    for (uint8_t i = 0; i < 4; i++) {
      LEDS_set(leds, 2, i, 0);
    }
  }
}

// triggers on ANY key off, used for 1-16 off's
void button_key_off_any(uint8_t key) {
  printf("off any %d\n", key);
  if (key > 3) {
    // 1-16 off
    if (mode_jump_mash == MODE_MASH) {
      // 1-16 off (mash mode)
      // remove momentary fx
      fx_toggle[key - 4] = false;
      do_update_fx(key - 4);
    }
  }
}

void button_key_on_single(uint8_t key) {
  printf("on %d\n", key);
  if (key < 4) {
    // TODO:
    // highlight toggle mode
    if (key == KEY_A) {
      if (mode_samp_bank) {
        // TODO: check this....
        LEDS_set(leds, 2, 2, 1);
        LEDS_set(leds, 2, 3, 0);
      };
    }
  } else {
    // 1-16
    if (mode_jump_mash == MODE_JUMP) {
      // 1-16 (jump mode)
      // do jump
      key_do_jump(key - 4);
    } else if (mode_jump_mash == MODE_MASH) {
      // 1-16 (mash mode)
      // do momentary fx
      fx_toggle[key - 4] = true;
      do_update_fx(key - 4);
    }
  }
}

void button_key_on_double(uint8_t key1, uint8_t key2) {
  printf("on %d+%d\n", key1, key2);
  if (key1 == KEY_SHIFT && key2 > 3) {
    // S+H
    if (mode_jump_mash == MODE_JUMP) {
      // S+H (jump mode)
      // toggles fx
      fx_toggle[key2 - 4] = !fx_toggle[key2 - 4];
      bool on = fx_toggle[key2 - 4];
      do_update_fx(key2 - 4);
    } else if (mode_jump_mash == MODE_MASH) {
      // S+H (mash mode)
      // does jump
      key_do_jump(key2 - 4);
    }
  } else if (key1 > 3 && key2 > 3) {
    // H+H
    if (mode_jump_mash == MODE_JUMP) {
      // retrigger

      debounce_quantize = 0;
      retrig_first = true;
      retrig_beat_num = random_integer_in_range(8, 24);
      retrig_timer_reset =
          96 * random_integer_in_range(1, 4) / random_integer_in_range(2, 12);
      float total_time = (float)(retrig_beat_num * retrig_timer_reset * 60) /
                         (float)(96 * sf->bpm_tempo);
      if (total_time > 2.0f) {
        total_time = total_time / 2;
        retrig_timer_reset = retrig_timer_reset / 2;
      }
      if (total_time > 2.0f) {
        total_time = total_time / 2;
        retrig_beat_num = retrig_beat_num / 2;
        if (retrig_beat_num == 0) {
          retrig_beat_num = 1;
        }
      }
      if (total_time < 0.25f) {
        total_time = total_time * 2;
        retrig_beat_num = retrig_beat_num * 2;
        if (retrig_beat_num == 0) {
          retrig_beat_num = 1;
        }
      }

      retrig_vol_step = 1.0 / ((float)retrig_beat_num);
      printf("retrig_beat_num=%d,retrig_timer_reset=%d,total_time=%2.3fs\n",
             retrig_beat_num, retrig_timer_reset, total_time);
      retrig_ready = true;
    }
  } else if (key1 == KEY_A) {
    // A
    if (key2 == KEY_B) {
      // A+B
      mode_samp_bank = 0;
    } else if (key2 == KEY_C) {
      // A+C
      mode_samp_bank = 1;
    } else if (key2 > 3) {
      if (mode_samp_bank == 0) {
        // A+H (sample  mode)
        // select sample
        fil_current_bank_next = fil_current_bank_sel;
        fil_current_id_next =
            ((key2 - 4) % (file_list[fil_current_bank_next].num / 2)) * 2;
        fil_current_change = true;
      } else {
        // A+H (bank mode)
        // select bank
        if (file_list[key2 - 4].num > 0) {
          fil_current_bank_sel = key2 - 4;
        }
      }
    }
  }
}

void button_handler(ButtonMatrix *bm) {
  if (key_total_pressed == 0) {
    key_timer++;
  }
  if (key_timer == 40 && key_pressed_num > 0) {
    printf("combo: ");
    for (uint8_t i = 0; i < key_pressed_num; i++) {
      printf("%d ", key_pressed[i]);
    }
    printf("\n");
    key_timer = 0;
    key_pressed_num = 0;
  }

  // read the latest from the queue
  ButtonMatrix_read(bm);

  // check queue for buttons that turned off
  for (uint8_t i = 0; i < bm->off_num; i++) {
    LEDS_set(leds, LED_PRESS_FACE, bm->off[i], 0);
    key_total_pressed--;
    key_on_buttons[bm->off[i]] = 0;
    button_key_off_any(bm->off[i]);
    // printf("turned off %d\n", bm->off[i]);
    if (key_held_on && (bm->off[i] == key_held_num)) {
      printf("off held %d\n", bm->off[i]);
      button_key_off_held(bm->off[i]);

      key_held_on = false;
      // TODO:
      // use the last key pressed that is still held as the new hold
      if (key_total_pressed > 0) {
        uint16_t *indexes =
            sort_int16_t(key_on_buttons, BUTTONMATRIX_BUTTONS_MAX);
        key_held_on = true;
        key_held_num = indexes[BUTTONMATRIX_BUTTONS_MAX - 1];
        free(indexes);
      } else {
        key_num_presses = 0;
        for (uint8_t i = 0; i < BUTTONMATRIX_BUTTONS_MAX; i++) {
          key_on_buttons[i] = 0;
        }
      }
    } else if (key_held_on) {
      printf("off %d+%d\n", key_held_num, bm->off[i]);
    } else {
      printf("off %d\n", bm->off[i]);
    }
  }

  // check queue for buttons that turned on
  bool key_held = false;
  for (uint8_t i = 0; i < bm->on_num; i++) {
    LEDS_set(leds, LED_PRESS_FACE, bm->on[i], 2);
    key_total_pressed++;
    if (!key_held_on) {
      key_held_on = true;
      key_held_num = bm->on[i];
      button_key_on_single(bm->on[i]);
    } else {
      button_key_on_double(key_held_num, bm->on[i]);
    }
    key_pressed[key_pressed_num] = bm->on[i];
    key_pressed_num++;
    key_timer = 0;

    // keep track
    key_num_presses++;
    key_on_buttons[bm->on[i]] = key_num_presses;
  }
}

// ### combo knobs
//
// - [ ]  *X → all probabilities (chaos)*
// - [ ]  *Y →* filter
// - [ ]  *Z →* volume
// - [ ]  *S* + *X* → tempo
// - [ ]  *S* + *Y →* pitch
// - [ ]  *S* + *Z* → gate
// - [ ]  *A* + *X* → distortion level
// - [ ]  *A* + *Y →* distortion wet
// - [ ]  *A* + *Z* → saturation wet
// - [ ]  *B* + *X* → probability jump away
// - [ ]  *B* + *Y →* probability reverse
// - [ ]  *B* + *Z* → probability probability retrig
// - [ ]  *C* + *X* → probability tunnel
// - [ ]  *C* + *Y →* probability pitch down
// - [ ]  *C* + *Z* → probability jump stay
//
// ### combo buttons
//
// - [ ]  *H* → JUMP: do jump, MASH: do fx (momentary)
// - [ ]  *H* + *H* → JUMP: retrig depending on location
// - [ ]  *S* + *H* → JUMP: do fx (toggle), MASH: do jump
// - [ ]  *S* → n/a
// - [ ]  *S* + *A* → toggle punch-in/jump mode
// - [ ]  *S* + *B* → toggle mute
// - [ ]  *S* + *C* → toggle playback
// - [ ]  *A* → show current bank (dim) + sample (bright)
// - [ ]  *A* + *B →* select bank mode
// - [ ]  *A* + *C →* select sample mode
// - [ ]  *A* + *H* → select bank/sample (depending on mode)
// - [ ]  *B* → n/a
// - [ ]  *B* + *A →* toggle play sequence
// - [ ]  *B* + *C →* toggle record sequence
// - [ ]  *B* + *H* → select sequence
// - [ ]  *C* → n/a
// - [ ]  *C* + *A →* load from save slot
// - [ ]  *C* + *B* → save into save slot
// - [ ]  *C* + *H* → select save slot
//
// ### visualization
//
// - [ ]  A light: bright on playback
// - [ ]  B light: blink on sequence recording, bright on sequence playing
// - [ ]  C light:
