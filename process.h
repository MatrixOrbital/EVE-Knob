#ifndef PROCESS_H
#define PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

void MakeScreen_Calibrate(void);
void MakeScreen_Gauge(uint16_t Val);
uint32_t Load_JPG(uint32_t BaseAdd, uint32_t Options, char *filename); 

#ifdef __cplusplus
}
#endif

#endif
