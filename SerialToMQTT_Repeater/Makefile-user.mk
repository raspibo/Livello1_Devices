## Local build configuration
## Parameters configured here will override default and ENV values.
## Uncomment and change examples:

#Add your source directories here separated by space
MODULES = app
# EXTRA_INCDIR = include

## ESP_HOME sets the path where ESP tools and SDK are located.
## Windows:
# ESP_HOME = c:/Espressif

## MacOS / Linux:
ESP_HOME = $(HOME)/git/esp-open-sdk

## SMING_HOME sets the path where Sming framework is located.
## Windows:
# SMING_HOME = c:/tools/sming/Sming 

# MacOS / Linux
SMING_HOME = $(HOME)/git/Sming/Sming

# esptool2 path
ESPTOOL2 ?= /home/pi/git/esptool2/esptool2

## COM port parameter is reqruied to flash firmware correctly.
## Windows: 
# COM_PORT = COM3

# MacOS / Linux:
COM_PORT = /dev/ttyAMA0

### Vecchi parametri
## Com port speed (Program transfer)
#COM_SPEED	= 115200
#
## Com port speed serial (& python r/w)
#COM_SPEED_SERIAL	= 115200

### Nuovi parametri
## COM port parameters
# Default COM port speed (generic)
COM_SPEED ?= 115200

# Default COM port speed (used for flashing)
COM_SPEED_ESPTOOL ?= $(COM_SPEED)

# *** Anche impostata, il programma non sembra leggersela, tranne per la console python ***
# Default COM port speed (used in code)
#COM_SPEED_SERIAL  ?= $(COM_SPEED)
COM_SPEED_SERIAL  ?= 9600

## Configure flash parameters (for ESP12-E and other new boards):
# SPI_MODE = dio

## SPIFFS options
DISABLE_SPIFFS = 1
# SPIFFs Location
#SPIFF_FILES = web/build
# we will use global WiFi settings from Eclipse Environment Variables, if possible

## Altri parametri
#RBOOT_ENABLED ?= 0
#SPI_SPEED ?= 40
#SPI_MODE ?= qio
#SPI_SIZE ?= 512K

# Test provati 2017.4.19 # Non vanno 80, 512,1,4,8
## Flash parameters
# SPI_SPEED = 40, 26, 20, 80
#SPI_SPEED ?= 40
# SPI_MODE: qio, qout, dio, dout
# *** Questa impostazione riguarda l'ultima versione di ESP8266 ESP-01, V3.0 ? ***
SPI_MODE ?= dout
# SPI_SIZE: 512K, 256K, 1M, 2M, 4M
#SPI_SIZE ?= 1M


ifdef MQTT_USERNAME
	USER_CFLAGS += -DMQTT_USERNAME=\"$(MQTT_USERNAME)\" -DMQTT_PWD=\"$(MQTT_PWD)\"
endif

ifdef MQTT_HOST
	USER_CFLAGS += -DMQTT_HOST=\"$(MQTT_HOST)\"
endif

ifdef MQTT_PORT
	USER_CFLAGS += -DMQTT_PORT=$(MQTT_PORT)
endif

### Direi che sta` roba per adesso non serve ###
# (Ho messo un cancelletto davanti a tutte le opzioni, successive
# a quessta frase. Eliminandolo, ce ne sarebbero un paio (o tre) che si
# attiverebbero)
## We need rBoot in order to be able to run bigger Flash roms.
#
##### overridable rBoot options ####
### use rboot build mode
#RBOOT_ENABLED ?= 1
### enable big flash support (for multiple roms, each in separate 1mb block of flash)
#RBOOT_BIG_FLASH ?= 1
### two rom mode (where two roms sit in the same 1mb block of flash)
##RBOOT_TWO_ROMS  ?= 1
### size of the flash chip
#SPI_SIZE  ?= 4M
