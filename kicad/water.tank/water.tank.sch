EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text GLabel 3850 2450 0    50   Input ~ 0
REST
Text GLabel 3850 2550 0    50   Input ~ 0
ADC
Text GLabel 3850 2650 0    50   Input ~ 0
CH_PD
Text GLabel 3850 2750 0    50   Input ~ 0
IO16
Text GLabel 3850 2850 0    50   Input ~ 0
IO14
Text GLabel 3850 2950 0    50   Input ~ 0
IO12
Text GLabel 3850 3050 0    50   Input ~ 0
IO13
Text GLabel 3850 3150 0    50   Input ~ 0
VCC
$Comp
L Connector:Conn_01x08_Female J1
U 1 1 627FC915
P 4450 2750
F 0 "J1" H 4478 2726 50  0001 L CNN
F 1 "ESP8266" H 4478 2680 50  0001 L CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical" H 4450 2750 50  0001 C CNN
F 3 "~" H 4450 2750 50  0001 C CNN
	1    4450 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 2450 4250 2450
Wire Wire Line
	3850 2550 4250 2550
Wire Wire Line
	3850 2650 4250 2650
Wire Wire Line
	3850 2750 4250 2750
Wire Wire Line
	3850 2850 4050 2850
Wire Wire Line
	3850 2950 3950 2950
Wire Wire Line
	3850 3050 4250 3050
Text GLabel 5500 3150 2    50   Input ~ 0
GND
Text GLabel 5500 3050 2    50   Input ~ 0
IO15
Text GLabel 5500 2950 2    50   Input ~ 0
IO02
Text GLabel 5500 2850 2    50   Input ~ 0
IO00
Text GLabel 5500 2750 2    50   Input ~ 0
IO04
Text GLabel 5500 2650 2    50   Input ~ 0
IO05
Text GLabel 5500 2550 2    50   Input ~ 0
RXD
Text GLabel 5500 2450 2    50   Input ~ 0
TXD
$Comp
L Connector:Conn_01x08_Female ESP8266
U 1 1 628017AD
P 4850 2850
F 0 "ESP8266" H 4878 2826 50  0000 L CNN
F 1 "ESP8266" H 4742 2317 50  0001 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical" H 4850 2850 50  0001 C CNN
F 3 "~" H 4850 2850 50  0001 C CNN
	1    4850 2850
	-1   0    0    1   
$EndComp
Wire Wire Line
	5500 3050 5050 3050
Wire Wire Line
	5500 2950 5050 2950
Wire Wire Line
	5500 2550 5050 2550
Wire Wire Line
	5500 2450 5050 2450
$Comp
L Connector:Conn_01x04_Female OLED1
U 1 1 628020C7
P 4850 3800
F 0 "OLED1" H 4650 4000 50  0000 L CNN
F 1 "OLED1" H 4650 4050 50  0001 L CNN
F 2 "Connector_JST:JST_EH_B4B-EH-A_1x04_P2.50mm_Vertical" H 4850 3800 50  0001 C CNN
F 3 "~" H 4850 3800 50  0001 C CNN
	1    4850 3800
	-1   0    0    1   
$EndComp
Text GLabel 5550 3900 2    50   Input ~ 0
VCC
Text GLabel 5550 3800 2    50   Input ~ 0
GND
Text GLabel 5550 3700 2    50   Input ~ 0
SCL
Text GLabel 5550 3600 2    50   Input ~ 0
SDA
Text GLabel 3850 3900 0    50   Input ~ 0
VCC
Text GLabel 3850 3800 0    50   Input ~ 0
RX-TRIG
Text GLabel 3850 3700 0    50   Input ~ 0
TX-ECHO
Text GLabel 3850 3600 0    50   Input ~ 0
GND
Wire Wire Line
	5550 3900 5050 3900
Wire Wire Line
	5550 3800 5050 3800
Wire Wire Line
	4250 3900 3850 3900
Wire Wire Line
	4250 3800 4050 3800
Wire Wire Line
	4250 3700 3950 3700
Wire Wire Line
	4250 3600 3850 3600
Wire Wire Line
	3950 2950 3950 3700
Connection ~ 3950 2950
Wire Wire Line
	3950 2950 4250 2950
Connection ~ 3950 3700
Wire Wire Line
	3950 3700 3850 3700
Wire Wire Line
	4050 2850 4050 3800
Connection ~ 4050 2850
Wire Wire Line
	4050 2850 4250 2850
Connection ~ 4050 3800
Wire Wire Line
	4050 3800 3850 3800
$Comp
L pspice:CAP 100nf1
U 1 1 62833051
P 7150 3850
F 0 "100nf1" V 6835 3850 50  0000 C CNN
F 1 "CAP" V 6926 3850 50  0000 C CNN
F 2 "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm" H 7150 3850 50  0001 C CNN
F 3 "~" H 7150 3850 50  0001 C CNN
	1    7150 3850
	0    1    1    0   
$EndComp
$Comp
L Regulator_Linear:LD0186D2T50TR U1
U 1 1 6283EFBA
P 6900 3250
F 0 "U1" H 6900 3617 50  0001 C CNN
F 1 "LD33V" H 6900 3525 50  0000 C CNN
F 2 "Package_TO_SOT_THT:TO-220-3_Vertical" H 6900 3750 50  0001 C CNN
F 3 "https://www.st.com/resource/en/datasheet/ld1086.pdf" H 6900 3750 50  0001 C CNN
	1    6900 3250
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x02_Female 5V1
U 1 1 62840F2A
P 7150 2550
F 0 "5V1" H 7178 2526 50  0000 L CNN
F 1 "source" H 7178 2435 50  0001 L CNN
F 2 "Connector_JST:JST_EH_B2B-EH-A_1x02_P2.50mm_Vertical" H 7150 2550 50  0001 C CNN
F 3 "~" H 7150 2550 50  0001 C CNN
	1    7150 2550
	1    0    0    -1  
$EndComp
Text GLabel 6250 2550 0    50   Input ~ 0
5V
Wire Wire Line
	6250 2550 6300 2550
Text GLabel 6250 2650 0    50   Input ~ 0
GND
Wire Wire Line
	6250 2650 6350 2650
Wire Wire Line
	6300 2550 6300 3250
Wire Wire Line
	6300 3250 6500 3250
Connection ~ 6300 2550
Wire Wire Line
	6300 2550 6950 2550
Wire Wire Line
	6350 2650 6350 3550
Wire Wire Line
	6350 3550 6900 3550
Connection ~ 6350 2650
Wire Wire Line
	6350 2650 6950 2650
Text GLabel 7550 3250 2    50   Input ~ 0
VCC
Wire Wire Line
	7300 3250 7400 3250
Wire Wire Line
	3850 3150 4250 3150
Wire Wire Line
	5050 3150 5500 3150
Wire Wire Line
	6900 3550 6900 3850
Connection ~ 6900 3550
Wire Wire Line
	7400 3850 7400 3250
Connection ~ 7400 3250
Wire Wire Line
	7400 3250 7550 3250
Wire Notes Line
	5950 2400 7900 2400
Wire Notes Line
	7900 2400 7900 4100
Wire Notes Line
	7900 4100 5950 4100
Wire Notes Line
	5950 4100 5950 2400
Text Notes 6000 4050 0    50   ~ 0
Power Supply
$Comp
L Connector:Conn_01x04_Female SENSOR1
U 1 1 628034E6
P 4450 3700
F 0 "SENSOR1" H 4200 3400 50  0000 L CNN
F 1 "JSN" H 4342 3367 50  0001 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical" H 4450 3700 50  0001 C CNN
F 3 "~" H 4450 3700 50  0001 C CNN
	1    4450 3700
	1    0    0    -1  
$EndComp
Wire Wire Line
	5050 2850 5500 2850
Wire Wire Line
	5050 3600 5150 3600
Wire Wire Line
	5050 2750 5150 2750
Wire Wire Line
	5050 3700 5300 3700
Wire Wire Line
	5150 3600 5150 2750
Connection ~ 5150 3600
Wire Wire Line
	5150 3600 5550 3600
Connection ~ 5150 2750
Wire Wire Line
	5150 2750 5500 2750
Wire Wire Line
	5300 3700 5300 2650
Wire Wire Line
	5050 2650 5300 2650
Connection ~ 5300 3700
Wire Wire Line
	5300 3700 5550 3700
Connection ~ 5300 2650
Wire Wire Line
	5300 2650 5500 2650
$EndSCHEMATC
