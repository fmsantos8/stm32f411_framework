/* Host-only CPU shim. Device registers and peripheral drivers remain the ST SDK. */
#ifndef TEST_CORE_CM4_H
#define TEST_CORE_CM4_H
#include <stdint.h>
#define __I volatile const
#define __O volatile
#define __IO volatile
#define __IM volatile const
#define __OM volatile
#define __IOM volatile
#define __STATIC_INLINE static inline
#define __weak __attribute__((weak))
#define __packed __attribute__((packed))

extern uint32_t test_interrupt_mask;
static inline uint32_t __get_PRIMASK(void) { return test_interrupt_mask; }
static inline void __disable_irq(void) { test_interrupt_mask = 1; }
static inline void __set_PRIMASK(uint32_t mask) { test_interrupt_mask = mask; }
static inline void __DMB(void) {}
static inline void __DSB(void) {}
static inline void __ISB(void) {}
static inline uint32_t __LDREXW(volatile uint32_t *address) { return *address; }
static inline uint32_t __STREXW(uint32_t value, volatile uint32_t *address) { *address = value; return 0; }

#endif
