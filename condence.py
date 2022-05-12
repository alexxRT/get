
import RPi.GPIO as GPIO
import time 

dac = [26, 19, 13, 6, 5, 11, 9, 10]
led = [21, 20, 16, 12, 7, 8, 25, 24]

comp = 4
troyka = 17
MaxVoltage = 3.3
start_time = 0
end_time = 0

GPIO.setmode (GPIO.BCM)
GPIO.setup (dac, GPIO.OUT, initial = GPIO.LOW)
GPIO.setup (troyka, GPIO.OUT, initial = GPIO.LOW)
GPIO.setup (led, GPIO.OUT)
GPIO.setup (comp, GPIO.IN)

def dec2bin (value):
    return [int(element) for element in bin(value)[2:].zfill(8)]

def bin2dec (value):
    s = 0
    for i in range (8):
        s += value[i] * 2**(7 - i)
    return s

def adc_new():
        signal = [0, 0, 0, 0, 0, 0, 0, 0]
        for i in range (8):
            signal[i] = 1
            GPIO.output (dac, signal)
            time.sleep (0.01)
            comp_value = GPIO.input (comp)
            if comp_value == 0:
                signal [i] = 0
            else:
                signal [i] = 1

        return bin2dec(signal)

try:

    # creat a list for voltage values 
    value_list = []

    # put HIGH voltage on troyka module and define beginning of the exp
    GPIO.output (troyka, GPIO.HIGH)
    start_time = time.time ()
    curent_voltage = adc_new()

    #charging
    while (curent_voltage < 0.97*255):
        value_list.append (curent_voltage)
        curent_voltage = adc_new

    # put LOW voltage on troyka module
    GPIO.output (troyka, GPIO.LOW)

    # discharging
    while (curent_voltage > 0.03*255):
        value_list.append (curent_voltage)
        curent_voltage = adc_new()

    #end of the expirement
    end_time = time.time()
    exp_time = float('{:.2f}'.format(end_time - start_time))

    # printing exp parametrs 
    print ("Время эксперимента: ", exp_time)
    print ("Период измерения: ", exp_time / len(value_list))
    print ("Средняя частота дискретизации: ", len(value_list) / exp_time)
    print ("Шаг квантования: ", (float)((MaxVoltage) / 255))

    # values in the file
    with open ('exp_res.txt', 'w') as exp:
        for i in range (len(value_list)):
            print >> exp, value_list [i] 


finally:
    GPIO.output (dac, GPIO.LOW)
    GPIO.output (led, GPIO.LOW)

    GPIO.cleanup (dac)
    print ("GPIO clean up completed")










            