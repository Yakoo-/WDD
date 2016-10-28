#ifndef _ADC_CTRL_H_
#define _ADC_CTRL_H_

#define ADC_DEVICE_NAME     "adc"
#define ADC_DEVICE_PATH     "/dev/adc"
#define WDD_ADC_CHANNEL     0
#define ADC_CMD_SELMUX      0

#define R1      4.7 // voltage dividing resistor kOm
#define R2      4.7 // Vin = Vbat * R2 * (R1 + R2)
#define RDVI    (R2 / (R1 + R2))
#define ADC_AVERAGE_ERROR       0.04
#define DEFAULT_REPEATE_TIME    500
#define N_TIMES_AVERAGE         4

int   get_adc_value(int channel);
inline float get_voltage(int adc_value);
int   get_vol_level(float voltage);
int   process_adc(void);

#endif
