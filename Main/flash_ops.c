#include "flash_ops.h"
#include "stm32f4xx_hal.h"
#include <string.h>

// This function will automatically be copied to RAM by CubeIDE!
__attribute__((section(".RamFunc"), noinline))
static void ram_flash_program(uint32_t addr, uint64_t data) {
    // Critical flash programming - runs from RAM
    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= FLASH_PSIZE_DOUBLE_WORD;
    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint64_t*)addr = data;

    // Wait for completion (CPU stalls here)
    while (FLASH->SR & FLASH_SR_BSY) {
        // Keep compiler from optimizing this away
        __asm volatile ("nop");
    }

    // Clear error flags if any
    if (FLASH->SR & (FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR)) {
        FLASH->SR = FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR;
    }

    FLASH->CR &= (~FLASH_CR_PG);
}

// Write a 512-byte sector with USB-friendly timing
bool flash_write_sector(uint32_t addr, const uint8_t *data) {
    uint32_t primask;

    // Write in 8 chunks of 64 bytes each
    for (int chunk = 0; chunk < 8; chunk++) {
        // Disable interrupts for this chunk only
        primask = __get_PRIMASK();
        __disable_irq();

        // Unlock on first chunk
        if (chunk == 0) {
            HAL_FLASH_Unlock();
        }

        // Program 64 bytes (8 doublewords) using RAM function
        for (int i = 0; i < 8; i++) {
            uint32_t offset = (chunk * 64) + (i * 8);
            uint64_t word = *(uint64_t*)(data + offset);
            ram_flash_program(addr + offset, word);
        }

        // Re-enable interrupts between chunks
        __set_PRIMASK(primask);

        // Tiny delay to let USB interrupts run
        // ~10µs is enough
        for (volatile int i = 0; i < 20; i++) {
            __asm volatile ("nop");
        }
    }

    HAL_FLASH_Lock();
    return true;
}

// Simple read function (no RAM needed)
bool flash_read_sector(uint32_t addr, uint8_t *data) {
    // Direct memory read - flash is memory-mapped
    memcpy(data, (void*)addr, 512);
    return true;
}
