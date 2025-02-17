#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"     
#include "hardware/pwm.h"  
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

//ALUNO: Thalles Inácio Araújo
// MATRÍCULA: tic370101531

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define VRX 27
#define VRY 26
#define JOY_BUTTON 22
#define led_green 11
#define led_blue 12
#define led_red 13
#define BUTTON_A 5
static volatile uint32_t last_time = 0;
uint slice_blue;
uint slice_red;
int idx = 1, idx2 = 0;
bool estado;
bool estado_b = true;
ssd1306_t ssd;

uint pwm_init_gpio(uint gpio, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, wrap);
    
    pwm_set_enabled(slice_num, true);  
    return slice_num;  
}

static void gpio_irq_handler(uint gpio, uint32_t events){
    
    
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time > 600000){ 
        last_time = current_time; 
        
        if (gpio == JOY_BUTTON) {  
            gpio_put(led_green,!gpio_get(led_green));
                if(gpio_get(led_green)) {
                    estado = true;
                    idx2 == 3 ? idx2 = 1 : idx2++;
                } else {
                    estado = false;
                }
        } else if(gpio == BUTTON_A) {
            if(estado_b == true){
                pwm_set_enabled(slice_blue, false);
                pwm_set_enabled(slice_red, false);
                estado_b = false;
            } else {
                pwm_set_enabled(slice_blue, true);
                pwm_set_enabled(slice_red, true);
                estado_b = true;
            }
        }
    }
}


int main(){
    stdio_init_all();
    
    gpio_init(led_green);
    gpio_set_dir(led_green,GPIO_OUT);

    gpio_init(JOY_BUTTON);
    gpio_set_dir(JOY_BUTTON,GPIO_IN);
    gpio_pull_up(JOY_BUTTON);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A,GPIO_IN);
    gpio_pull_up(BUTTON_A);

    adc_init();

    adc_gpio_init(VRX);
    adc_gpio_init(VRY);


    uint pwm_wrap = 4096;

    slice_blue = pwm_init_gpio(led_blue, pwm_wrap);
    slice_red = pwm_init_gpio(led_red, pwm_wrap);

    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); 
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); 
    gpio_pull_up(I2C_SDA); 
    gpio_pull_up(I2C_SCL); 
    
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); 
    ssd1306_config(&ssd); 
    ssd1306_send_data(&ssd); 

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    gpio_set_irq_enabled_with_callback(JOY_BUTTON, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    uint32_t last_display_update = 0;
    uint32_t update_interval = 5000;

    while(true) {
        
        uint32_t current_time = to_us_since_boot(get_absolute_time());
        
        adc_select_input(1);
        uint16_t vrx_value = adc_read();

        adc_select_input(0);
        uint16_t vry_value = adc_read();
        

        if (vrx_value <= 2048 && vrx_value >= 1875) { // Caso o joystick esteja centralizado no eixo x
            pwm_set_gpio_level(led_red, 0); // Desliga o LED vermelho
        } else {
            pwm_set_gpio_level(led_red, vrx_value); // Ajusta o PWM conforme a posição do eixo X
        }

        if (vry_value <= 2048 && vry_value >= 1950) { // Caso o joystick esteja centralizado no eixo y
            pwm_set_gpio_level(led_blue, 0); // Desliga o LED azul
        } else {
            pwm_set_gpio_level(led_blue, 4095 - vry_value); // Ajusta o PWM conforme a posição do eixo Y
        }
        
        uint8_t conv_x = (128 * vrx_value) / 4096;
        uint8_t conv_y = (64 * vry_value) / 4096;
            
            if(conv_y > 31) { // Como o eixo y é invertido no display, usei esse fator de conversão 
                conv_y = conv_y - 31;
                conv_y = 31 - conv_y;
            } else { 
                conv_y = (conv_y - 31) * -1;
                conv_y = 31 + conv_y;
            }
                
        if (current_time - last_display_update >= update_interval) {
            last_display_update = current_time;
                if(vrx_value <= 2048 && vrx_value >= 1875 && vry_value <= 2048 && vry_value >= 1950) { // caso o joystick esteja centralizado, o quadrado permanece centralizado no display 
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_square(&ssd, 64, 32);
                    ssd1306_send_data(&ssd);
                }else{
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_square(&ssd, conv_x, conv_y);
                    ssd1306_send_data(&ssd);
                }

                if(estado){ // variável usada pra detectar a interrupção (botão joystick - led verde)
                    
                    switch (idx2)
                    {
                        case 1:
                            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false); // maior
                            ssd1306_send_data(&ssd);
                            break;
                            

                        case 2:
                            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);    
                            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // padrão
                            ssd1306_send_data(&ssd);
                            break;
                        
                        case 3:
                            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);    
                            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);    
                            ssd1306_rect(&ssd, 6, 6, 116, 52, true, false); // menor
                            ssd1306_send_data(&ssd);
                            break;
                    }

                    
                } else {
                    ssd1306_fill(&ssd, false);
                    ssd1306_send_data(&ssd);
                }
        }
    }
}
