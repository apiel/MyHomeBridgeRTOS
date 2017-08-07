# Makefile for access_point example
PROGRAM=myhomebridge
# EXTRA_COMPONENTS=extras/dhcpserver extras/paho_mqtt_c extras/dht
EXTRA_COMPONENTS=extras/dhcpserver extras/paho_mqtt_c extras/dht extras/spiffs
FLASH_SIZE = 32

# spiffs configuration
# SPIFFS_BASE_ADDR = 0x200000
# SPIFFS_SIZE = 0x010000

# SPIFFS_SINGLETON = 0  # for run-time configuration

include ../esp-open-rtos/common.mk

# $(eval $(call make_spiffs_image,files))
