// was_pressed is a latch: set on the key-down transition, cleared by the main
// loop after every simulation tick. This way a quick tap can never be lost,
// even if the key is released before the next tick runs.
typedef struct {
    b32 is_down;
    b32 changed;
    b32 was_pressed;
} Button;

enum {
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_COUNT,
};

typedef struct {
    Button buttons[BUTTON_COUNT];
} Input;

#define pressed(b)  (input->buttons[b].was_pressed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)
#define is_down(b)  (input->buttons[b].is_down)
