{
  "targets": [
    {
      "target_name": "panel",
	  "sources": [ 
		  "src/panel.cc", 
		  "src/crc/crc.cc",
		  "src/gpio.cc",
		  "src/gpioclass.h",
		  "src/uart/uart.cc",
		  "src/timer/timer.cc",
		  "src/modbus/modbus_master.cc",
		  "src/modbus/modbus_slave.cc",
		  "src/timer/timer.h",
		  "src/modbus/modbus.h",
		  "src/modbus/modbus_master.h",
		  "src/modbus/modbus_slave.h",
		  "src/uart/uart.h",
		  "src/app.h",
		  "src/_config_lib_.h", 
		  "src/_config_cpu_.h", 
		  "src/raspberry.h",
		  "src/arch.h",
		  "src/crc/crc.h", 
		  "src/uc_libdefs.h"		  
		  ],
		  
      "include_dirs": [
		  "<!(node -e \"require('nan')\")",
		"src", "src/uart", "src/modbus","src/crc","src/timer"
		  
      ]
    }
  ]
}
