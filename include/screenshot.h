#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Where screenshot_process() saves the screen, as a binary PPM image. */
#define SCREENSHOT_FILE "/tmp/cmdisplay.ppm"

/**
 * @brief Starts listening for SIGUSR1: "kill -USR1 $(pidof cmdisplay)" saves
 * what the screen shows (popups included) to SCREENSHOT_FILE. Used to check
 * screens, for example translations, without being in front of the PDU.
 */
void screenshot_init(void);

/**
 * @brief Saves the screenshot if one was requested. Call from the LVGL loop:
 * the signal handler only sets a flag, as LVGL is not signal safe.
 */
void screenshot_process(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCREENSHOT_H */
