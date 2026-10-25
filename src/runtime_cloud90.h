#ifndef RUNTIME_CLOUD90_H
#define RUNTIME_CLOUD90_H
#include <stdint.h>
#include <stddef.h>
void runtime_cloud90_start(void *image, size_t size);
void runtime_cloud90_input(uint32_t *buttons, uint8_t *steering, uint8_t *accelerator);
#endif
