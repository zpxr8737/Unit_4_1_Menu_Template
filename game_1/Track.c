#include "Track.h"
#include "LCD.h"
#include <math.h>
#include "sprites.h"
#include "stm32l4xx_hal.h"

// Track params
typedef struct {
    float camera_z;        // world-space Z of the camera (grows as player moves forward)
    float speed;           // current speed in world-units/second
    uint8_t is_accelerating; // toggled by BTN2: 1 = throttle on, 0 = coasting
} track_state_t;
static track_state_t state;

// Fruit params
typedef struct {
    int32_t seg_idx;      // which segment the fruit sits on
    uint8_t fruit_type;   // 0=APPLE, 1=ORANGE, 2=WATERMELON
    int8_t lane;         // -1 = left lane, +1 = right lane
    uint8_t collected;    // 0 = active, 1 = collected
} FruitEntry_t;
static FruitEntry_t fruits[MAX_FRUIT_COUNT];

// Fruit Counters
static int32_t num_fruits = 0;
static int32_t fruits_collected = 0;

// Segment data
static float sC[MAX_SEGS];          // curvature value per segment
static int32_t num_segs = 0;           // segments built in current track

// Fruit sprites
static const uint8_t * const fruit_sprites[3] = {(const uint8_t *)appleSprite,(const uint8_t *)orangeSprite, (const uint8_t *)watermelonSprite};

// Curvature at the player's current segment
// Set once per frame before the scanline loop
static float current_curvature = 0.0f;

// Segment index of camera, recomputed every frame
static int32_t player_seg = 0;

// Pre-computed curvature accumulation for DRAW_DIST+2 segments ahead
static float seg_curve_acc[DRAW_DIST + 2];

// Lap variables
static uint8_t current_lap = 1;
static uint32_t lap_start_ms = 0;
static uint8_t race_complete = 0;
static uint32_t best_lap_ms = 0;

// Easing functions
static float easein(float a, float b, float t) {
    return a + (b - a) * t * t * t;
}

static float easeout(float a, float b, float t) {
    return a + (b - a) * (1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t));
}

// Add fruits to fruits[] for every road section
static void place_fruit(int32_t section_start, int32_t section_end) {
    if (num_fruits >= MAX_FRUIT_COUNT) return;

    int32_t mid_seg = (section_start + section_end) / 2;

    fruits[num_fruits].seg_idx = mid_seg; // segment in the middle of section
    fruits[num_fruits].fruit_type = num_fruits % 3; // cycle through 3 fruits
    fruits[num_fruits].lane = (num_segs % 2 == 0) ? -1 : 1; // alternate between left and right lane
    fruits[num_fruits].collected = 0; // flag if user drives over fruit
    num_fruits++;
}

// Segment building
static void add_seg(float c) {
    if (num_segs >= MAX_SEGS) return;
    sC[num_segs] = c;
    num_segs++;
}

static void add_straight(int32_t n) {
    int32_t section_start = num_segs;
    for (int32_t i = 0; i < n; i++)
        add_seg(0.0f);
    place_fruit(section_start, num_segs);
}

// Enter: segments easing in to full curvature c
// Hold:  segments at constant curvature c
// Exit_segs: segments easing back to straight
// c: curvature magnitude. Positive = curves right, negative = left.
static void add_curve(int32_t enter, int32_t hold, int32_t exit_segs, float c) {
    int32_t section_start = num_segs;

    for (int32_t i = 1; i <= enter; i++)
        add_seg(easein(0.0f, c, (float)i / (float)enter));

    for (int32_t i = 0; i < hold; i++)
        add_seg(c);

    for (int32_t i = 1; i <= exit_segs; i++)
        add_seg(easeout(c, 0.0f, (float)i / (float)exit_segs));
    
    place_fruit(section_start, num_segs);
}

void track_build(void) {
    num_segs = 0;
    add_straight(30);
    add_curve(15, 40, 15, 0.3f);
    add_straight(20);
    add_curve(15, 40, 15, -0.5f);
    add_straight(40);
    add_curve(10, 35, 10, 0.4f);
    add_straight(20);
    add_curve(10, 25, 10, -0.7f);
    add_straight(20);
    add_curve(10, 30, 10, 0.5f);
    add_straight(30);
}


// Wrapper function to draw horizontal strips
static inline void draw_hstrip(int16_t x_start, int16_t y, int16_t width, uint8_t colour) {
    if (width <= 0) return;

    // Clamp to left screen edge - reduce width by however far x is off-screen
    if (x_start < 0) {
        width += x_start;
        x_start = 0;
    }

    LCD_Draw_Rect((uint16_t)x_start, (uint16_t)y, (uint16_t)width, 1, colour, 1);
}

// Called in race_engine_init()
void track_init(void) {
    state.camera_z = 0.0f;
    state.speed = 0.0f;
    state.is_accelerating = 0;
    
    current_curvature = 0.0f;
    num_segs = 0;
    
    num_fruits = 0;
    fruits_collected = 0;
    
    current_lap = 1;
    lap_start_ms = HAL_GetTick();
    race_complete = 0;

    track_build();
}

// Check for fruit collision every frame
static void track_update_collision(float player_x) {
    if (num_segs == 0) return;

    // Check the segment immediately ahead
    int32_t check_seg = (player_seg + 1) % num_segs;

    for (int32_t t = 0; t < num_fruits; t++) {
        if (fruits[t].collected)          continue;   // already collected
        if (fruits[t].seg_idx != check_seg) continue; // not on this segment

        // Lateral collision - compare normalised positions of player and fruit (with scaler)
        float fruit_x = (float)fruits[t].lane * FRUIT_LANE_OFFSET;
        float dist = player_x - fruit_x;

        // Threshold within 0.3 to collect
        if (abs(dist) < 0.30f) {
            fruits[t].collected = 1;
            fruits_collected++;
        }
    }
}

// Update current lap and best lap time
static void track_update_lap(void) {
    if (race_complete) return;
    if (num_segs == 0)  return;

    // Compute amount of laps done
    float lap_length = (float)(num_segs * SEG_LEN);
    float laps_done_f = state.camera_z / lap_length;
    uint8_t laps_done = (uint8_t)laps_done_f;   // cast to integer

    // laps_done is 0 on lap 1, 1 on lap 2, 2 on lap 3
    uint8_t new_lap = laps_done + 1;

    if (new_lap > current_lap) {
        // Record completed lap time
        uint32_t completed_lap_ms = HAL_GetTick() - lap_start_ms;
        if (best_lap_ms == 0 || completed_lap_ms < best_lap_ms)
            best_lap_ms = completed_lap_ms;      // save fastest lap time

        // Player has crossed a lap boundary
        current_lap = new_lap;
        lap_start_ms = HAL_GetTick();   // reset lap timer

        if (current_lap > NUM_LAPS) race_complete = 1;
    }
}

// Called in race_engine_update to update track state
void track_update(uint32_t dt_ms, uint8_t btn2_pressed, float player_x) {
    // Hold BTN2 to accelerate, release to coast down
    state.is_accelerating = btn2_pressed;

    float dt = (float)dt_ms * 0.001f; // ms to seconds

    if (state.is_accelerating) {
        state.speed += ACCEL * dt;
        if (state.speed > MAX_SPEED)
            state.speed = MAX_SPEED;
    } else {
        state.speed -= DECEL * dt;
        if (state.speed < 0.0f)
            state.speed = 0.0f;
    }

    // Update camera Z position
    state.camera_z += state.speed * dt;
        
    track_update_lap();

    track_update_collision(player_x);
}

void track_draw(float player_x) {
    float pos_in_seg = 0.0f;

    if (num_segs > 0) {
        // Current camera segment and position within that segment
        player_seg = (int32_t)(state.camera_z / (float)SEG_LEN) % num_segs;
        pos_in_seg = fmodf(state.camera_z, SEG_LEN) / SEG_LEN;

        // Curvature of current segment
        current_curvature = sC[player_seg];

        // Partial negation
        float xoff = 0.0f;
        float dxoff = -sC[player_seg] * pos_in_seg;

        // Curvature accumulation
        for (int32_t i = 0; i <= DRAW_DIST + 1; i++) {
            seg_curve_acc[i] = xoff;
            xoff += dxoff;
            dxoff += sC[(player_seg + i) % num_segs];
        }

    } else {
        // Straight road if no track is built
        current_curvature = 0.0f;
        for (int32_t i = 0; i <= DRAW_DIST + 1; i++)
            seg_curve_acc[i] = 0.0f;
    }

    // Denominator for the perspective scale: total scanline rows in the road section.
    const int32_t TOTAL_DY = (int32_t)(SCREEN_HEIGHT - HORIZON_Y);  // = 120
    
    // Scanline-first track renderer
    for (int16_t y = HORIZON_Y; y < SCREEN_HEIGHT; y++) {
        // dy: distance below the horizon for this scanline
        // +1 so the first road row (y == HORIZON_Y) is not zero-width
        int32_t dy = (int32_t)(y - HORIZON_Y) + 1;
    
        // fPerspective: 0.0 at the horizon, 1.0 at the bottom of the screen
        float f_perspective = (float)dy / (float)TOTAL_DY;

        // Perspective-scaled widths
        float road_w = (float)(ROAD_WIDTH * f_perspective + (0.1*SCREEN_WIDTH));
        int16_t road_hw = 0.5*road_w;   // Half the road width at the screen's bottom row (pixels)
        int16_t kerb_w = 0.1*road_w;   // Full width of each kerb strip at the bottom row (pixels)
        int16_t dash_hw = 0.04*road_w;   // Half the width of the centre-line dash at the bottom row
        if (dash_hw < 1) dash_hw = 1;

        // Curvature offset for this scanline
        // Linear interpolation
        float segs_ahead = (float)DRAW_DIST / (float)dy;
        int32_t seg_lo = (int32_t)segs_ahead;
        if (seg_lo >= DRAW_DIST) seg_lo = DRAW_DIST - 1;
        float frac = segs_ahead - (float)seg_lo;
        float curve_offset = seg_curve_acc[seg_lo] + frac * (seg_curve_acc[seg_lo + 1] - seg_curve_acc[seg_lo]);

        // Camera offset: player_x shifts the vanishing point left/right
        // Scaled by road_hw so the offset is proportional to road width at this scanline
        int16_t cam_offset = (int16_t)(player_x * road_hw);
        int16_t vanish_x = CENTRE_X - cam_offset + (curve_offset * CURVE_SCALE);        

        // Derived pixel boundaries
        int16_t road_l = vanish_x - road_hw;
        int16_t road_r = vanish_x + road_hw;
        int16_t kerb_ll = road_l - kerb_w;
        int16_t kerb_rr = road_r + kerb_w;

        float inv_perspective = 1.0f - f_perspective;

        // Kerb - high frequency, moderate compression (pow=2)
        // sinf > 0 = red, sinf <= 0 = white
        float kerb_arg = (KERB_SINE_FREQ * powf(inv_perspective, KERB_SINE_POW)) + (state.camera_z * KERB_SCROLL_SPEED);
        float kerb_sine = sinf(kerb_arg);
        uint8_t kerb_col = kerb_sine > 0.0f ? COLOUR_IDX_KERB_RED : COLOUR_IDX_KERB_WHITE;

        // Grass - low frequency, heavy compression (pow=3)
        // sinf > 0 = grass A, sinf <= 0 = grass B
        float grass_arg = (GRASS_SINE_FREQ * powf(inv_perspective, GRASS_SINE_POW)) + (state.camera_z  * GRASS_SCROLL_SPEED);
        float grass_sine = sinf(grass_arg);
        uint8_t grass_col = grass_sine > 0.0f ? COLOUR_IDX_GRASS_A : COLOUR_IDX_GRASS_B;

        // Dash shares KERB_SCROLL_SPEED so it scrolls at the same rate as kerb
        // DASH_THRESHOLD controls dash length vs gap length within each cycle
        uint8_t show_dash = (kerb_sine > DASH_THRESHOLD) && (dash_hw > 0);

        // Left grass
        // From x=0 to the outer edge of the left kerb.
        draw_hstrip(0, y, kerb_ll, grass_col);

        // Left kerb
        draw_hstrip(kerb_ll, y, kerb_w, kerb_col);

        // Road surface
        draw_hstrip(road_l, y, road_hw * 2, COLOUR_IDX_ROAD);

        // Centre-line dashes
        if (show_dash) {
            draw_hstrip(vanish_x - dash_hw, y, dash_hw * 2, COLOUR_IDX_KERB_WHITE);
        }
        // Right kerb
        draw_hstrip(road_r, y, kerb_w, kerb_col);

        // Right grass
        // From the outer edge of the right kerb to x=SCREEN_WIDTH-1.
        draw_hstrip(kerb_rr, y, SCREEN_WIDTH - kerb_rr, grass_col);
    }    
    
    // Segment-first sprite renderer
    for (int32_t t = num_fruits - 1; t >= 0; t--) {
        
        int32_t seg_idx = fruits[t].seg_idx;
        int32_t segs_ahead = (seg_idx - player_seg + num_segs) % num_segs;

        if (segs_ahead < 1 || segs_ahead > SPRITE_DRAW_DIST) continue;
        if (fruits[t].collected) continue;

        // Project to screen
        int32_t dy = DRAW_DIST / segs_ahead;
        if (dy < 1) dy = 1;
        if (dy > TOTAL_DY) dy = TOTAL_DY;
        
        // Screen Y coordinate of the middle of sprite
        int16_t screen_y = (int16_t)(HORIZON_Y + dy);
        
        // Perspective scaler
        float f_perspective = (float)dy / (float)TOTAL_DY;
        float ROAD_W_seg = (float)ROAD_WIDTH * f_perspective + 0.1f * (float)SCREEN_WIDTH;
        int16_t road_hw_seg = (int16_t)(0.5f * ROAD_W_seg);
        
        // Linear interpolation
        int32_t seg_lo = segs_ahead;
        if (seg_lo >= DRAW_DIST) seg_lo = DRAW_DIST - 1;
        float frac = segs_ahead - (float)seg_lo;
        float curve_offset = seg_curve_acc[seg_lo] + frac * (seg_curve_acc[seg_lo + 1] - seg_curve_acc[seg_lo]);

        // Camera offset - affected by both player and curvatrue
        int16_t vanish_x_seg = CENTRE_X - (int16_t)(player_x * (float)road_hw_seg) + (int16_t)(curve_offset * CURVE_SCALE);

        // Scale
        uint8_t scale = (uint8_t)((float)road_hw_seg / 20.0f);
        if (scale < SPRITE_MIN_SCALE) continue;
        if (scale > SPRITE_MAX_SCALE) scale = SPRITE_MAX_SCALE;

        // FRUIT_LANE_OFFSET=0.4 places fruit 40% across the half-road width
        int16_t lane_offset_px = (int16_t)(FRUIT_LANE_OFFSET * (float)road_hw_seg * (float)fruits[t].lane);
        // Lane position - offset from vanish_x toward left or right
        int16_t fruit_centre_x = vanish_x_seg + lane_offset_px;

        // Screen cords
        int16_t sprite_w = (int16_t)(FRUIT_SPRITE_W * scale);
        int16_t sprite_h = (int16_t)(FRUIT_SPRITE_H * scale);

        int16_t x0 = fruit_centre_x - sprite_w / 2;
        int16_t y0 = screen_y - sprite_h;

        LCD_Draw_Sprite_Scaled((uint16_t)x0, (uint16_t)y0, FRUIT_SPRITE_H, FRUIT_SPRITE_W, fruit_sprites[fruits[t].fruit_type], scale);
    }
}

float track_get_curvature(void) {
    return current_curvature;
}

float track_get_speed(void) {
    return state.speed;
}

float track_get_camera_z(void) {
    return state.camera_z;
}

int32_t track_get_fruits_collected(void) {
    return fruits_collected; 
}

int32_t track_get_total_fruits(void) {
    return num_fruits;
}

uint8_t track_get_current_lap(void) {
    // Clamp to NUM_LAPS so display never shows lap 4
    return (current_lap > NUM_LAPS) ? NUM_LAPS : current_lap;
}

uint32_t track_get_lap_time_ms(void) {
    return HAL_GetTick() - lap_start_ms;
}

uint8_t track_is_race_complete(void) {
    return race_complete;
}

uint32_t track_get_best_lap_ms(void) {
    return best_lap_ms;
}