#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>

#include "ccd_ctrl.h"
#include "adc_ctrl.h"
#include "oled_ctrl.h"
#include "button_ctrl.h"
#include "main.h"
#include "fftw3.h"


pthread_mutex_t repeate_lock;
pthread_cond_t  repeate_cond;
unsigned int repeate; 


#define ONCE 1
#define NONE 0

#define N 1024      			// fft-length
#define G 256     			// gary 0-255
#define N_part (N - CCD_EDGE_MASK)

#define lg 18   			//highpass filter length
#define var 43  			//the count-weft value
#define eq_rm_long 3
#define otsu_rm_long 30
#define cnt_rm_long 3

static int rmeq,rmotsu,rmcnt;

unsigned char data[N]={0};
unsigned char eq[N]={0}; 		//histeq result & otsu input
unsigned char eq_tmp[N]={0};
unsigned char two[N]={0}; 		// otsu outputfloat fftmp[N2][2]
unsigned char zone[N]={0};		//otsu select zone
unsigned char thr=0;  			//otsu var
unsigned int otsu_sum=0; 		//sum_area
unsigned int line=0;  			//weft line
float result=0.0;

fftwf_complex *fftmp;  		//fft-output
unsigned char ifftmp[N]={0};  		//ifft-array
unsigned char cnt[N]={0};       	// the count-array

float sample_process(uint *);
void histeq();
void otsu();
void rmchip_data();    			// qumaoci removing
void rmchip_OTSU_CNT();    		// qumaoci removing
void count();

void fft(void);
void hfliter(void);
void ifft(void);

#define BAT_IMAGE_WIDTH     (BATTERY_LEVEL_NUM + 4)
#define BATTERY_LEVEL_NUM   7   // 0 ~ 6
#define BAT_IMG_BLANK_INX   0
#define BAT_IMG_HEAD_INX    1 
#define BAT_IMG_FULL_INX    2 
#define BAT_IMG_VOL_INX     3
/*                        blank head  full  vol */
static const vol_image[] = { 0, 0x1c, 0x7f, 0x41 };

void fresh_battery(void)
{
    int i = 0, col = 0;
    char bits; 

    while(1){
        int vol_lev = process_adc();

        /* print battery image on screen */
        for (i = 0; i < BAT_IMAGE_WIDTH; i++){
            switch (i) {
            case 0:
                bits = vol_image[BAT_IMG_BLANK_INX];
                break;
            case 1:
                bits = vol_image[BAT_IMG_HEAD_INX];
                break;
            case 2:
            case (BAT_IMAGE_WIDTH - 1):
                bits = vol_image[BAT_IMG_FULL_INX];
                break;

            default:
                bits = (BATTERY_LEVEL_NUM - (i - 2)) > vol_lev ? vol_image[BAT_IMG_VOL_INX] : vol_image[BAT_IMG_FULL_INX];
                break;
            }

            OLED_WrBits(OLED_COL_NUM - BAT_IMAGE_WIDTH + i, 0, bits);
        }
        DEBUG_NUM(vol_lev);
        sleep(30);
    }
}

void print_line(int line)
{
    char line_str[20] = " lines: ";
    char num_str[20] = {0};

    sprintf(num_str, "%d", line);
    strcat(line_str, num_str);

    /* hard code, TBD */
    OLED_P6x8Str(5 * 6, 0, line_str);
}

void clear_line(void)
{
    char line_str[20] = "           ";
    OLED_P6x8Str(5 * 6, 0, line_str);
}


void print_ccd_data(unsigned int * pixels)
{
    unsigned int col = 0, row = 0, i = 0;
    float sresult = 0.0;
    char bits_pattern[OLED_BYTES_NUM] = {0};  // 7 * 128
    unsigned int threshold_row = 0;

    /* pixel number for each sector */
    static const uint pns = (CCD_MAX_PIXEL_CNT - CCD_EDGE_MASK * 2) / OLED_COL_NUM;

    /* average of pixel values in one sector */
    uint aop = 0;

    for (col = 0; col < OLED_COL_NUM ; col++){
        aop = 0;
        for (i = 0; i < pns; i++)
            aop += pixels[CCD_EDGE_MASK + col * pns  + i];

        /* count pixels average value and compress */
        aop = aop / pns / CCD_COMPRESS_RATE;    
        threshold_row = OLED_ROW_NUM - (aop / OLED_ROW_NUM) - 1;

        for (row = 0; row < threshold_row; row++)
            OLED_WrBits(col, row + CCD_BOTTOM_MASK, 0);

        OLED_WrBits(col, threshold_row + CCD_BOTTOM_MASK, ( 0xff00 >> (aop % OLED_ROW_NUM) ) & 0xff);

        for (row = threshold_row + 1; row < OLED_ROW_NUM; row++)
            OLED_WrBits(col, row + CCD_BOTTOM_MASK, 0xff);
    }
}

float sample_process(unsigned int *pixels)
{
    int i = 0;

    if (NULL == fftmp)
        fftmp = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * CCD_MAX_PIXEL_CNT);

    // copy data, cutting off edge;
    for (i = CCD_EDGE_MASK; i < (CCD_MAX_PIXEL_CNT - CCD_EDGE_MASK); i++)
        data[i] = (unsigned char)(pixels[i] >> 2);    // 10bits to 8bits

    histeq(data);
    for (i = 0; i < CCD_MAX_PIXEL_CNT; i++){
        if ( (i < CCD_EDGE_MASK) || (i >= (CCD_MAX_PIXEL_CNT - CCD_EDGE_MASK)) )
            eq[i] = 0;
        eq_tmp[i] = eq[i];
    }

    otsu(eq);
    for (i = 0; i < CCD_MAX_PIXEL_CNT; i++)
        eq_tmp[i] = zone[i] ? 0 : eq_tmp[i];

    fft();
    hfliter();
    ifft();
    for (i = 0; i < CCD_MAX_PIXEL_CNT; i++)
        ifftmp[i] = zone[i] ? 0 : ifftmp[i];

    count(ifftmp);

    result = (line * 1.0) / ( 1.307 * otsu_sum * 1.0 / CCD_MAX_PIXEL_CNT);

    return result;
}

#define G   256

void histeq(unsigned char h[N])
{
    int i=0;
    int j=0;
    unsigned char num[G]={0};
    float prob[G]={0.0};
    float cumu[G]={0.0};
    //zhifangtu tongji
    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        num[h[i]] = num[h[i]]+1;    //imhist
    }
    //zhifangtu guiyi
    for(i=0;i<G;i++)
    {
        prob[i] = num[i] / (N_part * 1.0);
    }
    //zhifangtu leiji
    for(i=0;i<G;i++)
    {
        for(j=0;j<=i;j++)
        {
            cumu[i] = cumu[i] + prob[j];
        }
    }
    //yingshe
    for(i=0;i<G;i++)
    {
        cumu[i] = cumu[i]*255 + 0.5;
    }
    //junheng output
    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        eq[i]=cumu[(h[i])];
    }
}

void otsu(unsigned char o[N])
{
    int i=0;
    int j=0;
    float w0, w1, u0tmp, u1tmp, u0, u1;
    float u;
    float varTmp, varMax = 0;

    unsigned char num[G]={0};
    float prob[G]={0.0};

    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        num[eq[i]] = num[eq[i]]+1;    //imhist
    }

    for(i=0;i<G;i++)
    {
        prob[i] = num[i] / (N_part * 1.0);
    }

	for(i = 0; i < G; i++)
	{
		w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = varTmp = 0;

		for(j = 0; j < G; j++)
		{
			if(j <= i) 
			{
				w0 += prob[j];
				u0tmp += j * prob[j];
			}
			else       
			{
				w1 += prob[j];
				u1tmp += j * prob[j];
			}
		}

		u0 = u0tmp / w0;		
		u1 = u1tmp / w1;		
		u = u0tmp + u1tmp;		
		varTmp = w0 * (u0 - u)*(u0 - u) + w1 * (u1 - u)*(u1 - u);

		if(varTmp > varMax)
		{
			varMax = varTmp;
			thr = i;
		}
	}

    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        if(eq[i]<= thr) two[i]=1;
        else    two[i]=0;
    }
    rmchip_data(two,rmeq);  //data rm chip
    rmchip_OTSU_CNT(two,rmotsu,zone); //otsu range
    for(i=0;i<CCD_EDGE_MASK;i++)
    {
        zone[i]=1;
    }
    for(i=N_part;i<N;i++)
    {
        zone[i]=1;
    }
    for(i=0;i<N;i++)
    {
        if(!zone[i])    otsu_sum++;
    }
}

void rmchip_data(unsigned char input[N],unsigned int n)     // USE OTSU TO RM CHIP OF THE DATA
{
    unsigned int i=0;
    unsigned int u=0;
    unsigned int st=0;
    unsigned int end=0;

    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        if(i==(N_part-1))
        {
            end=N_part-1;
        }
        else if(input[i+1]!=input[i])
        {
            if(input[i]==1)   end=i;
            if(input[i]==0)   st=i+1;
        }

        if((end - st) < n && (st < end))
        {
            if(st==CCD_EDGE_MASK)
            {
                for(u=st;u<end+1;u++)
                {
                    eq_tmp[u]=eq_tmp[end+1];
                }
            }
            else if(end==(N_part-1))
            {
                for(u=st;u<end+1;u++)
                {
                    eq_tmp[u]=eq_tmp[st-1];
                }
            }
            else
            {
                for(u=st;u<end+1;u++)
                {
                    eq_tmp[u]=(eq_tmp[st-1]+eq_tmp[end+1])/2;
                }
            }
        }

        if((end==st) && (input[st]==1))
        {
            if(st==CCD_EDGE_MASK) eq_tmp[u]=eq_tmp[end+1];
            else if(end==N_part-1) eq_tmp[u]=eq_tmp[st-1];
            else eq_tmp[u]=(eq_tmp[st-1]+eq_tmp[end+1])/2;
        }
    }
}

void rmchip_OTSU_CNT(unsigned char input[N],unsigned int n,unsigned char output[N])
{
    unsigned int i=0;
    unsigned int u=0;
    unsigned int st=0;
    unsigned int end=0;

    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        output[i]=input[i];
    }

    for(i=0;i<N;i++)
    {
        if(i==(N-1))
        {
            end=N-1;
        }
        else if(input[i+1]!=input[i])
        {
            if(input[i]==1)   end=i;
            if(input[i]==0)   st=i+1;
        }

        if((end - st) < n && (st < end))
        {
            if(st==CCD_EDGE_MASK)
            {
                for(u=st;u<end+1;u++)
                {
                    output[u]=0;
                }
            }
            else if(end==(N-1))
            {
                for(u=st;u<end+1;u++)
                {
                    output[u]=0;
                }
            }
            else
            {
                for(u=st;u<end+1;u++)
                {
                    output[u]=0;
                }
            }
        }

        if((end==st) && (input[st]==1))
        {
            output[st]=0;
        }
    }
}

void fft()
{
    int i;
    double *in = NULL;
    fftwf_complex *out = NULL;
    fftwf_plan p;

    in  = (double *)fftwf_malloc(sizeof(double) * N);
    out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * N);

    for(i=0;i<N;i++)
    {
        in[i] = eq_tmp[i];
    }

    p = fftwf_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
    fftwf_execute(p); /* repeat as needed*/

    memcpy(fftmp,out,sizeof(fftwf_complex) * N);

    fftwf_destroy_plan(p);
    fftwf_free(in);
    fftwf_free(out);
}

void hfliter()
{
    memset(fftmp[0],0,sizeof(double)*lg);
    memset(fftmp[1],0,sizeof(double)*lg);
}

void ifft()
{
    int i;
    double *out = NULL;
    fftwf_complex *in = NULL;
    fftwf_plan p;

    out  = (double *)fftwf_malloc(sizeof(double) * N);
    in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * N);

    memcpy(in,fftmp,sizeof(fftwf_complex) * N);

    p = fftwf_plan_dft_c2r_1d(N, in, out, FFTW_ESTIMATE);
    fftwf_execute(p); /* repeat as needed*/
    for(i = 0;i < N;i++)
    {
        ifftmp[i]=out[i]/N + 0.50;
    }
    fftwf_destroy_plan(p);
    fftwf_free(in);
    fftwf_free(out);
}

void count(unsigned char c[N])
{
    int i=0;
    int k=0;
    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        if(ifftmp[i]<=var)
            cnt[i]=0;
        else
            cnt[i]=1;
    }
    rmchip_OTSU_CNT(cnt,rmcnt,cnt);  //qumaoci
    for(i=CCD_EDGE_MASK;i<N_part;i++)
    {
        if(cnt[i]!=cnt[i+1])
            k=k+1;
    }
    line=k/2;
}

int main (int argc, char ** argv)
{
    int fd_ccd = -1, i = 0, ret = 0, vol_lev = 0;
    unsigned int pixels[CCD_MAX_PIXEL_CNT];
    const unsigned int buffer_len = sizeof(pixels);
    pthread_t ppid_button;
    pthread_t ppid_voltage;

    /* process input value */
    if (argc == 2)
        if (!strcmp(argv[1], "auto"))
            repeate = DEFAULT_REPEATE_TIME;
        else
            repeate = atoi(argv[1]);

    /* hard code, TBD */
    OLED_P6x8Str(0, 0, " WDD ");

    ret = pthread_create(&ppid_button, NULL, (void *)select_button, NULL);
    if (ret != 0)
        Error("Failed to create button select thread!");

    ret = pthread_create(&ppid_voltage, NULL, (void *)fresh_battery, NULL);
    if (ret != 0)
        Error("Failed to create button select thread!");

    fd_ccd = open(CCD_DEV_NAME, O_RDONLY);
    if (fd_ccd < 0)
        Error("Failed to open device: " CCD_DEV_NAME "!\n");

    while (1){
        pthread_mutex_lock( &repeate_lock );
        while (repeate == 0)
            pthread_cond_wait( &repeate_cond, &repeate_lock );

        clear_line();
        unsigned int line_tmp = 0;
        for (i = 0; i < repeate; i++){

            ret = read(fd_ccd, pixels, buffer_len);

            sample_process(pixels);

            line_tmp += line;

            /* fresh ccd data on oled screen */
            print_ccd_data(pixels);
            usleep(300000);
        }
        print_line(line_tmp / repeate);

        repeate = 0;
        pthread_mutex_unlock( &repeate_lock );
    }
    return 0;
}
