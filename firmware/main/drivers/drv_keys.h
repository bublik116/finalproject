#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool menu;
    bool up;
    bool down;
    bool ok;
} keys_state_t;

void drv_keys_init(void);
void drv_keys_read(keys_state_t *out);

#ifdef __cplusplus
}
#endif
